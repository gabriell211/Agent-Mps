#include "IppCollector.hpp"
#include "../core/Logger.hpp"
#include <curl/curl.h>
#include <cstring>
#include <map>
#include <algorithm>
#include <chrono>
#include <sstream>

static const uint16_t IPP_OP_GET_PRINTER_ATTRS = 0x000B;
static const uint16_t IPP_VERSION_11            = 0x0101;

static const uint8_t  TAG_CHARSET        = 0x47;
static const uint8_t  TAG_NATURAL_LANG   = 0x48;
static const uint8_t  TAG_URI            = 0x45;
static const uint8_t  TAG_KEYWORD        = 0x44;
static const uint8_t  TAG_NAME_WITHOUT   = 0x42;
static const uint8_t  TAG_INTEGER        = 0x21;
static const uint8_t  TAG_ENUM           = 0x23;
static const uint8_t  TAG_OP_ATTRS       = 0x01;
static const uint8_t  TAG_PRINTER_ATTRS  = 0x04;
static const uint8_t  TAG_END_OF_ATTRS   = 0x03;

static void push16(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back((v >> 8) & 0xFF);
    buf.push_back(v & 0xFF);
}
static void push32(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back((v >> 24) & 0xFF);
    buf.push_back((v >> 16) & 0xFF);
    buf.push_back((v >> 8)  & 0xFF);
    buf.push_back(v & 0xFF);
}
static void push_attr(std::vector<uint8_t>& buf, uint8_t tag, const std::string& name, const std::string& val) {
    buf.push_back(tag);
    push16(buf, (uint16_t)name.size());
    for (char c : name) buf.push_back((uint8_t)c);
    push16(buf, (uint16_t)val.size());
    for (char c : val) buf.push_back((uint8_t)c);
}

std::vector<uint8_t> IppCollector::build_get_printer_attrs_request(const std::vector<std::string>& attrs,
                                                                     const std::string& printer_uri) {
    std::vector<uint8_t> buf;
    push16(buf, IPP_VERSION_11);
    push16(buf, IPP_OP_GET_PRINTER_ATTRS);
    push32(buf, 1);
    buf.push_back(TAG_OP_ATTRS);
    push_attr(buf, TAG_CHARSET, "attributes-charset", "utf-8");
    push_attr(buf, TAG_NATURAL_LANG, "attributes-natural-language", "en");
    push_attr(buf, TAG_URI, "printer-uri", printer_uri);

    if (!attrs.empty()) {
        bool first = true;
        for (auto& a : attrs) {
            push_attr(buf, TAG_KEYWORD,
                      first ? "requested-attributes" : "",
                      a);
            first = false;
        }
    }
    buf.push_back(TAG_END_OF_ATTRS);
    return buf;
}

static size_t curl_write_bytes(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = reinterpret_cast<std::vector<uint8_t>*>(userdata);
    auto* bytes = reinterpret_cast<uint8_t*>(ptr);
    out->insert(out->end(), bytes, bytes + size * nmemb);
    return size * nmemb;
}

std::vector<uint8_t> IppCollector::send_ipp(const std::string& ip, int port, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> response;
    CURL* curl = curl_easy_init();
    if (!curl) return response;

    std::string url = "http://" + ip + ":" + std::to_string(port) + "/ipp/print";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.data());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)payload.size());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_bytes);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    struct curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/ipp");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

    CURLcode rc = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    if (rc != CURLE_OK || status < 200 || status >= 300) {
        Log::debug("ipp: curl erro em {}: {}", ip, curl_easy_strerror(rc));
        response.clear();
    }

    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    return response;
}

std::map<std::string, std::string> IppCollector::parse_ipp_response(const std::vector<uint8_t>& data) {
    std::map<std::string, std::string> out;
    if (data.size() < 8) return out;

    size_t i = 8; 
    std::string last_name;

    while (i < data.size()) {
        uint8_t tag = data[i++];
        if (tag == TAG_END_OF_ATTRS) break;
        if (tag <= 0x0F) continue;  

        if (i + 2 > data.size()) break;
        uint16_t name_len = ((uint16_t)data[i] << 8) | data[i+1];
        i += 2;
        if (i + name_len > data.size()) break;

        std::string name(data.begin() + i, data.begin() + i + name_len);
        i += name_len;

        if (i + 2 > data.size()) break;
        uint16_t val_len = ((uint16_t)data[i] << 8) | data[i+1];
        i += 2;
        if (i + val_len > data.size()) break;

        std::string val(data.begin() + i, data.begin() + i + val_len);
        if ((tag == TAG_INTEGER || tag == TAG_ENUM) && val_len == 4) {
            const uint32_t number = (static_cast<uint32_t>(data[i]) << 24) |
                                    (static_cast<uint32_t>(data[i + 1]) << 16) |
                                    (static_cast<uint32_t>(data[i + 2]) << 8) |
                                    static_cast<uint32_t>(data[i + 3]);
            val = std::to_string(number);
        }
        i += val_len;

        if (!name.empty()) last_name = name;
        if (!last_name.empty()) out[last_name] = val;
    }
    return out;
}

bool IppCollector::check(const std::string& ip, int port) {
    const std::string uri = "ipp://" + ip + ":" + std::to_string(port) + "/ipp/print";
    auto payload = build_get_printer_attrs_request({"printer-state"}, uri);
    auto resp    = send_ipp(ip, port, payload);
    return resp.size() >= 8;
}

PrinterStatus IppCollector::collect_status(const std::string& ip, int printer_id, int port) {
    PrinterStatus s;
    s.printer_id = printer_id;

    const std::string uri = "ipp://" + ip + ":" + std::to_string(port) + "/ipp/print";
    auto payload = build_get_printer_attrs_request({
        "printer-state",
        "printer-state-reasons",
        "printer-up-time",
        "printer-state-message"
    }, uri);
    auto resp = send_ipp(ip, port, payload);
    auto attrs = parse_ipp_response(resp);
    auto it = attrs.find("printer-state");
    if (it != attrs.end()) {
        if (!it->second.empty()) {
            char st = it->second[0];
            if (st == 3 || it->second == "3") s.status = "idle";
            else if (st == 4 || it->second == "4") s.status = "printing";
            else s.status = "stopped";
        }
    }

    it = attrs.find("printer-state-reasons");
    if (it != attrs.end()) s.status_message = it->second;

    it = attrs.find("printer-state-message");
    if (it != attrs.end() && !it->second.empty()) s.status_message = it->second;

    it = attrs.find("printer-up-time");
    if (it != attrs.end()) {
        try { s.uptime_s = std::stoll(it->second); } catch (...) {}
    }
    s.collected_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return s;
}

std::vector<Consumable> IppCollector::collect_consumables(const std::string& ip, int printer_id, int port) {
    std::vector<Consumable> out;
    const std::string uri = "ipp://" + ip + ":" + std::to_string(port) + "/ipp/print";
    auto payload = build_get_printer_attrs_request({"marker-levels", "marker-names", "marker-types", "marker-colors"}, uri);
    auto resp    = send_ipp(ip, port, payload);
    auto attrs   = parse_ipp_response(resp);

    long long ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    auto names  = attrs["marker-names"];
    auto levels = attrs["marker-levels"];
    auto types  = attrs["marker-types"];
    auto colors = attrs["marker-colors"];

    auto split = [](const std::string& s) {
        std::vector<std::string> v;
        std::istringstream ss(s);
        std::string tok;
        while (ss >> tok) v.push_back(tok);
        return v;
    };

    auto vnames  = split(names);
    auto vlevels = split(levels);
    auto vtypes  = split(types);
    auto vcolors = split(colors);

    for (size_t i = 0; i < vnames.size(); i++) {
        Consumable c;
        c.printer_id   = printer_id;
        c.name         = vnames[i];
        c.type         = i < vtypes.size()  ? vtypes[i]  : "toner";
        c.color        = i < vcolors.size() ? vcolors[i] : "";
        c.collected_at = ts;
        if (i < vlevels.size()) {
            try {
                c.level_pct = std::stoi(vlevels[i]);
                if (c.level_pct <= 5)  c.state = "empty";
                else if (c.level_pct <= 20) c.state = "low";
                else c.state = "ok";
            } catch (...) {}
        }
        out.push_back(c);
    }
    return out;
}
