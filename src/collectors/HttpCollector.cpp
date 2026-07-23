#include "HttpCollector.hpp"
#include "../core/Logger.hpp"
#include <curl/curl.h>
#include <regex>
#include <algorithm>

static size_t curl_write_str(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* out = reinterpret_cast<std::string*>(userdata);
    out->append(reinterpret_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

std::string HttpCollector::get_http(const std::string& url, int timeout_s) {
    std::string body;
    CURL* curl = curl_easy_init();
    if (!curl) return body;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout_s);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_str);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "santa-agent/1.0");

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK)
        Log::debug("http: erro em {}: {}", url, curl_easy_strerror(rc));

    curl_easy_cleanup(curl);
    return body;
}

std::string HttpCollector::fetch(const std::string& url, int timeout_s) {
    return get_http(url, timeout_s);
}

std::string HttpCollector::xml_value(const std::string& body, const std::string& tag) {
    std::string open  = "<" + tag + ">";
    std::string close = "</" + tag + ">";
    auto a = body.find(open);
    if (a == std::string::npos) {
        open = "<" + tag + " ";
        a = body.find(open);
        if (a == std::string::npos) return "";
        a = body.find('>', a);
        if (a == std::string::npos) return "";
        a++;
        close = "</" + tag + ">";
    } else {
        a += open.size();
    }
    auto b = body.find(close, a);
    if (b == std::string::npos) return "";
    return body.substr(a, b - a);
}

std::vector<std::string> HttpCollector::xml_values(const std::string& body, const std::string& tag) {
    std::vector<std::string> out;
    std::string open  = "<" + tag + ">";
    std::string close = "</" + tag + ">";
    size_t pos = 0;
    while ((pos = body.find(open, pos)) != std::string::npos) {
        pos += open.size();
        auto end = body.find(close, pos);
        if (end == std::string::npos) break;
        out.push_back(body.substr(pos, end - pos));
        pos = end + close.size();
    }
    return out;
}

Printer HttpCollector::collect_inventory_ews(const std::string& ip, const std::string& mfr) {
    Printer p;
    p.ip           = ip;
    p.manufacturer = mfr;

    if (mfr == "HP") {
        auto body = get_http("http://" + ip + "/DevMgmt/ProductConfigDyn.xml", 5);
        if (body.empty())
            body = get_http("https://" + ip + "/DevMgmt/ProductConfigDyn.xml", 5);

        p.model      = xml_value(body, "dd:ProductName");
        p.serial     = xml_value(body, "dd:SerialNumber");
        p.fw_version = xml_value(body, "dd:FirmwareRevision");
        p.hostname   = xml_value(body, "dd:Hostname");
        if (p.model.empty()) {
            body = get_http("http://" + ip + "/", 4);
            auto a = body.find("<title>"); auto b = body.find("</title>");
            if (a != std::string::npos && b != std::string::npos)
                p.model = body.substr(a + 7, b - a - 7);
        }
    }
    else if (mfr == "Ricoh") {
        auto body = get_http("http://" + ip + "/web/guest/en/websys/status/getConfiguration.cgi", 5);
        p.model      = xml_value(body, "model_name");
        p.serial     = xml_value(body, "serial_number");
        p.fw_version = xml_value(body, "engine_firmware");
    }
    else if (mfr == "Canon") {
        auto body = get_http("http://" + ip + "/portal_top.html", 4);
        auto a = body.find("<title>"); auto b = body.find("</title>");
        if (a != std::string::npos && b != std::string::npos)
            p.model = body.substr(a + 7, b - a - 7);
    }
    else if (mfr == "Xerox") {
        auto body = get_http("http://" + ip + "/webApp/statusPRINTER", 4);
        p.model      = xml_value(body, "model");
        p.serial     = xml_value(body, "serial");
    }
    else if (mfr == "Kyocera") {
        auto body = get_http("http://" + ip + "/startwlm/Start.htm", 4);
        auto a = body.find("<title>"); auto b = body.find("</title>");
        if (a != std::string::npos && b != std::string::npos)
            p.model = body.substr(a + 7, b - a - 7);
    }
    else {
        auto body = get_http("http://" + ip + "/", 4);
        auto a = body.find("<title>"); auto b = body.find("</title>");
        if (a != std::string::npos && b != std::string::npos)
            p.model = body.substr(a + 7, b - a - 7);
    }

    return p;
}

std::string HttpCollector::parse_hp_ews_status(const std::string& body) {
    auto it = body.find("StatusCategory");
    if (it == std::string::npos) return "";
    auto end = body.find('<', it);
    if (end == std::string::npos) return "";
    auto start = body.rfind('>', it);
    if (start == std::string::npos) return "";
    return body.substr(start + 1, end - start - 1);
}
