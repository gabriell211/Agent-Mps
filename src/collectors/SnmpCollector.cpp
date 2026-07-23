#include "SnmpCollector.hpp"
#include "../core/Logger.hpp"
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <chrono>
#include <cstring>
#include <map>
#include <algorithm>
#include <cctype>
#include <mutex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif

#define OID_SYS_DESCR       "1.3.6.1.2.1.1.1.0"
#define OID_SYS_OBJECTID    "1.3.6.1.2.1.1.2.0"
#define OID_SYS_UPTIME      "1.3.6.1.2.1.1.3.0"
#define OID_SYS_NAME        "1.3.6.1.2.1.1.5.0"
#define OID_SYS_LOCATION    "1.3.6.1.2.1.1.6.0"
#define OID_HR_MEMORY_SIZE  "1.3.6.1.2.1.25.2.2.0"
#define OID_PRT_GENERAL     "1.3.6.1.2.1.43.5.1.1"   
#define OID_PRT_SERIAL      "1.3.6.1.2.1.43.5.1.1.17.1"
#define OID_PRT_FW_VER      "1.3.6.1.2.1.43.5.1.1.16.1"
#define OID_PRT_STATUS      "1.3.6.1.2.1.43.18.1.1"  
#define OID_PRT_SUPPLIES    "1.3.6.1.2.1.43.11.1.1"   
#define OID_PRT_MARKER      "1.3.6.1.2.1.43.10.2.1"   
#define OID_SUPPLY_DESC     "1.3.6.1.2.1.43.11.1.1.6"
#define OID_SUPPLY_TYPE     "1.3.6.1.2.1.43.11.1.1.5"
#define OID_SUPPLY_COLOR    "1.3.6.1.2.1.43.12.1.1.4"
#define OID_SUPPLY_CAPACITY "1.3.6.1.2.1.43.11.1.1.8"
#define OID_SUPPLY_LEVEL    "1.3.6.1.2.1.43.11.1.1.9"
#define OID_TOTAL_PAGES     "1.3.6.1.2.1.43.10.2.1.4.1.1"
#define OID_IF_SPEED        "1.3.6.1.2.1.2.2.1.5"
#define OID_IF_TYPE         "1.3.6.1.2.1.2.2.1.3"
#define OID_IP_ADDR         "1.3.6.1.2.1.4.20.1.1"
#define OID_IP_MASK         "1.3.6.1.2.1.4.20.1.3"
#define OID_IP_DEFAULT_GW   "1.3.6.1.2.1.4.21.1.7.0.0.0.0"

static const std::map<std::string, std::map<std::string,std::string>> VENDOR_OIDS = {
    {"HP", {
        {"model",       "1.3.6.1.4.1.11.2.3.9.1.1.7.0"},
        {"serial",      "1.3.6.1.4.1.11.2.3.9.1.1.10.0"},
        {"fw_version",  "1.3.6.1.4.1.11.2.3.9.1.1.3.0"},
        {"asset_tag",   "1.3.6.1.4.1.11.2.3.9.1.1.17.0"},
        {"total_pages", "1.3.6.1.4.1.11.2.3.9.4.2.1.4.1.2.5"},
        {"color_pages", "1.3.6.1.4.1.11.2.3.9.4.2.1.4.1.2.6"},
    }},
    {"Ricoh", {
        {"model",       "1.3.6.1.4.1.367.3.2.1.2.1.5.0"},
        {"serial",      "1.3.6.1.4.1.367.3.2.1.2.1.4.0"},
        {"fw_version",  "1.3.6.1.4.1.367.3.2.1.2.1.6.0"},
        {"total_pages", "1.3.6.1.4.1.367.3.2.1.2.2.7.0"},
        {"color_pages", "1.3.6.1.4.1.367.3.2.1.2.2.6.0"},
    }},
    {"Canon", {
        {"model",       "1.3.6.1.4.1.1602.1.1.1.1.0"},
        {"serial",      "1.3.6.1.4.1.1602.1.1.1.5.0"},
        {"fw_version",  "1.3.6.1.4.1.1602.1.1.1.3.0"},
        {"total_pages", "1.3.6.1.4.1.1602.1.3.11.1.0"},
    }},
    {"Xerox", {
        {"model",       "1.3.6.1.4.1.253.8.53.3.2.1.3.1"},
        {"serial",      "1.3.6.1.4.1.253.8.53.3.2.1.7.1"},
        {"fw_version",  "1.3.6.1.4.1.253.8.53.13.2.1.6.1.1"},
        {"total_pages", "1.3.6.1.4.1.253.8.53.13.2.1.6.1.2"},
    }},
    {"Brother", {
        {"model",       "1.3.6.1.4.1.2435.2.3.9.1.1.7.0"},
        {"serial",      "1.3.6.1.4.1.2435.2.3.9.1.1.6.0"},
        {"fw_version",  "1.3.6.1.4.1.2435.2.3.9.1.1.5.0"},
        {"total_pages", "1.3.6.1.4.1.2435.2.3.9.4.2.1.5.5.1"},
    }},
    {"Kyocera", {
        {"model",       "1.3.6.1.4.1.1347.41.1.1.1.1.1.0"},
        {"serial",      "1.3.6.1.4.1.1347.41.1.1.1.1.11.0"},
        {"fw_version",  "1.3.6.1.4.1.1347.41.1.1.1.1.4.0"},
        {"total_pages", "1.3.6.1.4.1.1347.42.2.1.1.1.10.1"},
    }},
    {"Konica", {
        {"model",       "1.3.6.1.4.1.18334.1.1.1.5.7.2.1.0"},
        {"serial",      "1.3.6.1.4.1.18334.1.1.1.5.7.2.3.0"},
        {"total_pages", "1.3.6.1.4.1.18334.1.1.1.5.11.1.1.5.1"},
    }},
    {"Epson", {
        {"model",       "1.3.6.1.4.1.1248.1.1.3.1.5.0"},
        {"serial",      "1.3.6.1.4.1.1248.1.1.3.1.4.0"},
        {"fw_version",  "1.3.6.1.4.1.1248.1.1.3.1.7.0"},
    }},
    {"Lexmark", {
        {"model",       "1.3.6.1.4.1.641.1.1.0"},
        {"serial",      "1.3.6.1.4.1.641.1.2.0"},
        {"fw_version",  "1.3.6.1.4.1.641.1.3.0"},
        {"total_pages", "1.3.6.1.4.1.641.2.1.2.1.6.1"},
    }},
};

static std::once_flag snmp_initialization;

SnmpCollector::SnmpCollector(const SnmpConfig& cfg) : cfg(cfg) {
    std::call_once(snmp_initialization, [] {
#ifdef _WIN32
        WSADATA data{};
        WSAStartup(MAKEWORD(2, 2), &data);
#endif
        init_snmp("santa-agent");
    });
}

SnmpCollector::~SnmpCollector() {}

static netsnmp_session* open_session(const std::string& ip, const SnmpConfig& cfg) {
    netsnmp_session s;
    snmp_sess_init(&s);

    s.peername = const_cast<char*>(ip.c_str());
    s.remote_port = cfg.port;
    s.timeout = cfg.timeout * 1000000L;   // microsegundos
    s.retries = cfg.retries;

    if (cfg.version == "3") {
        s.version = SNMP_VERSION_3;
        s.securityName = const_cast<char*>(cfg.sec_name.c_str());
        s.securityNameLen = cfg.sec_name.size();
        if (s.securityNameLen == 0) return nullptr;
        s.securityLevel = SNMP_SEC_LEVEL_NOAUTH;

        if (!cfg.auth_pass.empty()) {
            if (cfg.auth_proto == "SHA") {
                s.securityAuthProto = usmHMACSHA1AuthProtocol;
                s.securityAuthProtoLen = USM_AUTH_PROTO_SHA_LEN;
            } else {
                s.securityAuthProto = usmHMACMD5AuthProtocol;
                s.securityAuthProtoLen = USM_AUTH_PROTO_MD5_LEN;
            }
            s.securityAuthKeyLen = USM_AUTH_KU_LEN;
            if (generate_Ku(s.securityAuthProto, s.securityAuthProtoLen,
                            reinterpret_cast<const u_char*>(cfg.auth_pass.data()), cfg.auth_pass.size(),
                            s.securityAuthKey, &s.securityAuthKeyLen) != SNMPERR_SUCCESS) {
                return nullptr;
            }
            s.securityLevel = SNMP_SEC_LEVEL_AUTHNOPRIV;
        }
        if (!cfg.priv_pass.empty()) {
            if (cfg.auth_pass.empty()) return nullptr;
            if (cfg.priv_proto == "DES") {
                s.securityPrivProto = usmDESPrivProtocol;
                s.securityPrivProtoLen = USM_PRIV_PROTO_DES_LEN;
            } else {
                s.securityPrivProto = usmAESPrivProtocol;
                s.securityPrivProtoLen = USM_PRIV_PROTO_AES_LEN;
            }
            s.securityPrivKeyLen = USM_PRIV_KU_LEN;
            if (generate_Ku(s.securityAuthProto, s.securityAuthProtoLen,
                            reinterpret_cast<const u_char*>(cfg.priv_pass.data()), cfg.priv_pass.size(),
                            s.securityPrivKey, &s.securityPrivKeyLen) != SNMPERR_SUCCESS) {
                return nullptr;
            }
            s.securityLevel = SNMP_SEC_LEVEL_AUTHPRIV;
        }
    }
    else if (cfg.version == "1") {
        s.version        = SNMP_VERSION_1;
        s.community      = (u_char*)cfg.community.c_str();
        s.community_len  = cfg.community.size();
    }
    else {
        s.version        = SNMP_VERSION_2c;
        s.community      = (u_char*)cfg.community.c_str();
        s.community_len  = cfg.community.size();
    }

    netsnmp_session* ss = snmp_open(&s);
    return ss;
}

static void close_session(netsnmp_session* ss) {
    if (ss) snmp_close(ss);
}

static bool tcp_port_open(const std::string& ip, int port, int timeout_ms = 800) {
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return false;
    u_long nonblocking = 1;
    ioctlsocket(sock, FIONBIO, &nonblocking);
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    const int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) != 1) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
        return false;
    }
    const int result = connect(sock, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    bool open = result == 0;
    if (!open) {
#ifdef _WIN32
        const int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS) {
#else
        if (errno == EINPROGRESS) {
#endif
            fd_set write_set;
            FD_ZERO(&write_set);
            FD_SET(sock, &write_set);
            timeval timeout{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
#ifdef _WIN32
            if (select(0, nullptr, &write_set, nullptr, &timeout) > 0) {
#else
            if (select(sock + 1, nullptr, &write_set, nullptr, &timeout) > 0) {
#endif
                int socket_error = 0;
#ifdef _WIN32
                int length = sizeof(socket_error);
#else
                socklen_t length = sizeof(socket_error);
#endif
                open = getsockopt(sock, SOL_SOCKET, SO_ERROR,
                                  reinterpret_cast<char*>(&socket_error), &length) == 0 && socket_error == 0;
            }
        }
    }
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
    return open;
}

std::string SnmpCollector::get_oid(const std::string& ip, const std::string& oid_str) {
    auto ss = open_session(ip, cfg);
    if (!ss) return "";

    netsnmp_pdu* pdu  = snmp_pdu_create(SNMP_MSG_GET);
    oid     anOID[MAX_OID_LEN];
    size_t  anOID_len = MAX_OID_LEN;

    if (!snmp_parse_oid(oid_str.c_str(), anOID, &anOID_len)) {
        snmp_pdu_free(pdu);
        close_session(ss);
        return "";
    }
    snmp_add_null_var(pdu, anOID, anOID_len);

    netsnmp_pdu* response = nullptr;
    int status = snmp_synch_response(ss, pdu, &response);
    std::string result;

    if (status == STAT_SUCCESS && response && response->errstat == SNMP_ERR_NOERROR) {
        auto* var = response->variables;
        if (var) {
            char buf[1024] = {};
            snprint_value(buf, sizeof(buf), var->name, var->name_length, var);
            result = buf;
            auto colon = result.find(": ");
            if (colon != std::string::npos)
                result = result.substr(colon + 2);
            if (!result.empty() && result.front() == '"') result = result.substr(1);
            if (!result.empty() && result.back()  == '"') result.pop_back();
        }
    }
    if (response) snmp_free_pdu(response);
    close_session(ss);
    return result;
}

long long SnmpCollector::get_oid_int(const std::string& ip, const std::string& oid_str) {
    auto val = get_oid(ip, oid_str);
    if (val.empty()) return 0;
    try { return std::stoll(val); } catch (...) { return 0; }
}

std::vector<std::pair<std::string,std::string>>
SnmpCollector::walk_oid(const std::string& ip, const std::string& base_oid) {
    std::vector<std::pair<std::string,std::string>> out;
    auto ss = open_session(ip, cfg);
    if (!ss) return out;

    oid root[MAX_OID_LEN];
    size_t rootlen = MAX_OID_LEN;
    if (!snmp_parse_oid(base_oid.c_str(), root, &rootlen)) {
        close_session(ss);
        return out;
    }

    oid name[MAX_OID_LEN];
    size_t namelen = rootlen;
    memcpy(name, root, rootlen * sizeof(oid));

    bool running = true;
    while (running) {
        netsnmp_pdu* pdu = snmp_pdu_create(SNMP_MSG_GETNEXT);
        snmp_add_null_var(pdu, name, namelen);
        netsnmp_pdu* response = nullptr;
        int status = snmp_synch_response(ss, pdu, &response);

        if (status != STAT_SUCCESS || !response || response->errstat != SNMP_ERR_NOERROR) {
            if (response) snmp_free_pdu(response);
            break;
        }
        auto* var = response->variables;
        if (!var || snmp_oid_compare(root, rootlen, var->name, var->name_length) != 0
            || !netsnmp_oid_is_subtree(root, rootlen, var->name, var->name_length)) {
            if (response) snmp_free_pdu(response);
            break;
        }

        char oid_buf[256] = {};
        snprint_objid(oid_buf, sizeof(oid_buf), var->name, var->name_length);
        char val_buf[1024] = {};
        snprint_value(val_buf, sizeof(val_buf), var->name, var->name_length, var);
        std::string val_str = val_buf;
        auto colon = val_str.find(": ");
        if (colon != std::string::npos) val_str = val_str.substr(colon + 2);
        if (!val_str.empty() && val_str.front() == '"') val_str = val_str.substr(1);
        if (!val_str.empty() && val_str.back()  == '"') val_str.pop_back();

        out.emplace_back(oid_buf, val_str);

        memcpy(name, var->name, var->name_length * sizeof(oid));
        namelen = var->name_length;
        snmp_free_pdu(response);
    }
    close_session(ss);
    return out;
}

int SnmpCollector::ping(const std::string& ip) {
    auto t0 = std::chrono::steady_clock::now();
    auto val = get_oid(ip, OID_SYS_UPTIME);
    if (val.empty()) return -1;
    auto t1 = std::chrono::steady_clock::now();
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
}

std::string SnmpCollector::detect_manufacturer(const std::string& sys_oid) {
    if (sys_oid.find("1.3.6.1.4.1.11") != std::string::npos)    return "HP";
    if (sys_oid.find("1.3.6.1.4.1.367") != std::string::npos)   return "Ricoh";
    if (sys_oid.find("1.3.6.1.4.1.1602") != std::string::npos)  return "Canon";
    if (sys_oid.find("1.3.6.1.4.1.253") != std::string::npos)   return "Xerox";
    if (sys_oid.find("1.3.6.1.4.1.2435") != std::string::npos)  return "Brother";
    if (sys_oid.find("1.3.6.1.4.1.1347") != std::string::npos)  return "Kyocera";
    if (sys_oid.find("1.3.6.1.4.1.18334") != std::string::npos) return "Konica";
    if (sys_oid.find("1.3.6.1.4.1.1248") != std::string::npos)  return "Epson";
    if (sys_oid.find("1.3.6.1.4.1.641") != std::string::npos)   return "Lexmark";
    return "";
}

Printer SnmpCollector::collect_inventory(const std::string& ip) {
    Printer p;
    p.ip = ip;

    // basico MIB-II
    std::string sys_desc = get_oid(ip, OID_SYS_DESCR);
    std::string sys_oid  = get_oid(ip, OID_SYS_OBJECTID);
    p.hostname    = get_oid(ip, OID_SYS_NAME);
    p.manufacturer = detect_manufacturer(sys_oid);

    p.serial     = get_oid(ip, OID_PRT_SERIAL);
    p.fw_version = get_oid(ip, OID_PRT_FW_VER);

    p.model = sys_desc;

    if (!p.manufacturer.empty())
        apply_vendor_oids(ip, p, p.manufacturer);

    const long long memory_kb = get_oid_int(ip, OID_HR_MEMORY_SIZE);
    if (memory_kb > 0) p.memory_mb = static_cast<int>(memory_kb / 1024);

    auto general = walk_oid(ip, OID_PRT_GENERAL);
    for (auto& [k, v] : general) {
        if (k.find(".1.1.10.1") != std::string::npos)  
            p.has_duplex = (v != "0" && !v.empty());
        if (k.find(".1.1.18.1") != std::string::npos)  
            p.has_scanner = true;
    }

    p.last_seen = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    return p;
}

void SnmpCollector::apply_vendor_oids(const std::string& ip, Printer& p, const std::string& mfr) {
    auto it = VENDOR_OIDS.find(mfr);
    if (it == VENDOR_OIDS.end()) return;
    auto& oids = it->second;

    auto fetch = [&](const std::string& key) -> std::string {
        auto oit = oids.find(key);
        if (oit == oids.end()) return "";
        return get_oid(ip, oit->second);
    };

    auto v = fetch("model");      if (!v.empty()) p.model      = v;
    v = fetch("serial");          if (!v.empty()) p.serial     = v;
    v = fetch("fw_version");      if (!v.empty()) p.fw_version = v;
    v = fetch("asset_tag");       if (!v.empty()) p.asset_tag  = v;
}

PrinterStatus SnmpCollector::collect_status(const std::string& ip, int printer_id) {
    PrinterStatus s;
    s.printer_id = printer_id;

    long long uptime_ticks = get_oid_int(ip, OID_SYS_UPTIME);
    s.uptime_s = uptime_ticks / 100;   
    auto status_val = get_oid_int(ip, "1.3.6.1.2.1.43.16.5.1.2.1.1");
    switch (status_val) {
        case 3:  s.status = "idle";     break;
        case 4:  s.status = "printing"; break;
        case 5:  s.status = "warming";  break;
        default: s.status = "unknown";  break;
    }

    auto alerts = walk_oid(ip, OID_PRT_STATUS);
    for (auto& [k, v] : alerts) {
        if (k.find(".1.1.7") != std::string::npos && !v.empty() && v != "0") {
            s.error_code    = v;
            s.status        = "error";
        }
        if (k.find(".1.1.9") != std::string::npos && !v.empty()) {
            s.status_message = v;
        }
    }

    s.collected_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return s;
}

std::vector<Consumable> SnmpCollector::collect_consumables(const std::string& ip, int printer_id) {
    std::vector<Consumable> out;
    long long ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    auto descriptions = walk_oid(ip, OID_SUPPLY_DESC);     // .6.x.y
    auto capacities   = walk_oid(ip, OID_SUPPLY_CAPACITY); // .8.x.y
    auto levels       = walk_oid(ip, OID_SUPPLY_LEVEL);    // .9.x.y
    auto types        = walk_oid(ip, OID_SUPPLY_TYPE);     // .5.x.y

    auto idx = [](const std::string& oid_str) -> std::string {
        auto dot = oid_str.rfind('.');
        return dot != std::string::npos ? oid_str.substr(dot + 1) : oid_str;
    };

    std::map<std::string, Consumable> by_idx;
    for (auto& [k, v] : descriptions) {
        auto i = idx(k);
        by_idx[i].name = v;
        by_idx[i].printer_id = printer_id;
        by_idx[i].collected_at = ts;
    }
    for (auto& [k, v] : capacities) {
        auto i = idx(k);
        try { by_idx[i].capacity_pages = std::stoi(v); } catch (...) {}
    }
    for (auto& [k, v] : levels) {
        auto i = idx(k);
        try {
            int lv = std::stoi(v);
            by_idx[i].remaining_pages = lv;
            if (by_idx[i].capacity_pages > 0) {
                by_idx[i].level_pct = (int)((double)lv / by_idx[i].capacity_pages * 100.0);
            }
            if (lv == -1) { by_idx[i].level_pct = 100; by_idx[i].state = "ok"; }
            else if (lv == -2) { by_idx[i].level_pct = -1; by_idx[i].state = "unknown"; }
            else if (by_idx[i].level_pct <= 5)  by_idx[i].state = "empty";
            else if (by_idx[i].level_pct <= 20) by_idx[i].state = "low";
            else                                 by_idx[i].state = "ok";
        } catch (...) {}
    }
    for (auto& [i, c] : by_idx) {
        std::string name_lc = c.name;
        std::transform(name_lc.begin(), name_lc.end(), name_lc.begin(), ::tolower);

        if (name_lc.find("black") != std::string::npos || name_lc.find("preto") != std::string::npos) {
            c.type = "toner"; c.color = "black";
        } else if (name_lc.find("cyan") != std::string::npos) {
            c.type = "toner"; c.color = "cyan";
        } else if (name_lc.find("magenta") != std::string::npos) {
            c.type = "toner"; c.color = "magenta";
        } else if (name_lc.find("yellow") != std::string::npos || name_lc.find("amarelo") != std::string::npos) {
            c.type = "toner"; c.color = "yellow";
        } else if (name_lc.find("drum") != std::string::npos) {
            c.type = "drum";
        } else if (name_lc.find("fuser") != std::string::npos || name_lc.find("fusor") != std::string::npos) {
            c.type = "fuser";
        } else if (name_lc.find("belt") != std::string::npos) {
            c.type = "belt";
        } else if (name_lc.find("waste") != std::string::npos) {
            c.type = "waste";
        } else if (name_lc.find("staple") != std::string::npos || name_lc.find("grampo") != std::string::npos) {
            c.type = "staple";
        } else if (name_lc.find("maintenance") != std::string::npos) {
            c.type = "maintenance_kit";
        } else {
            c.type = "other";
        }
        out.push_back(c);
    }

    std::string mfr;
    {
        auto sys_oid = get_oid(ip, OID_SYS_OBJECTID);
        mfr = detect_manufacturer(sys_oid);
    }
    if (!mfr.empty())
        apply_vendor_consumables(ip, out, printer_id, mfr);

    return out;
}

void SnmpCollector::apply_vendor_consumables(const std::string& ip, std::vector<Consumable>& out,
                                              int printer_id, const std::string& mfr) {

    (void)ip; (void)out; (void)printer_id; (void)mfr;
}

Counter SnmpCollector::collect_counters(const std::string& ip, int printer_id) {
    Counter c;
    c.printer_id = printer_id;
    c.total_pages = get_oid_int(ip, OID_TOTAL_PAGES);

    auto sys_oid = get_oid(ip, OID_SYS_OBJECTID);
    auto mfr     = detect_manufacturer(sys_oid);

    if (!mfr.empty()) {
        auto it = VENDOR_OIDS.find(mfr);
        if (it != VENDOR_OIDS.end()) {
            auto fetch_v = [&](const std::string& key) -> long long {
                auto oit = it->second.find(key);
                if (oit == it->second.end()) return 0;
                return get_oid_int(ip, oit->second);
            };
            auto tot = fetch_v("total_pages");
            if (tot > 0) c.total_pages = tot;
            c.color_pages = fetch_v("color_pages");
            c.mono_pages  = c.total_pages - c.color_pages;
        }
    }
    c.collected_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return c;
}

NetworkConfig SnmpCollector::collect_network(const std::string& ip, int printer_id) {
    NetworkConfig n;
    n.printer_id = printer_id;
    n.ipv4 = ip;

    auto masks = walk_oid(ip, OID_IP_MASK);
    for (auto& [k, v] : masks) {
        if (k.find(ip) != std::string::npos) { n.subnet = v; break; }
    }

    n.gateway = get_oid(ip, OID_IP_DEFAULT_GW);

    auto speeds = walk_oid(ip, OID_IF_SPEED);
    for (auto& [k, v] : speeds) {
        try {
            long long spd = std::stoll(v);
            if (spd > 0) { n.speed_mbps = (int)(spd / 1000000); break; }
        } catch (...) {}
    }

    n.uptime_s = get_oid_int(ip, OID_SYS_UPTIME) / 100;

    n.collected_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return n;
}

SecurityInfo SnmpCollector::collect_security(const std::string& ip, int printer_id) {
    SecurityInfo s;
    s.printer_id     = printer_id;
    s.snmp_version   = cfg.version;
    s.snmp_community = cfg.version != "3" ? (cfg.community == "public" ? "default-public" : "custom-configured") : "v3";

    struct CheckPort {
        int port;
        std::string name;
    };
    std::vector<CheckPort> checks = {
        {21,   "ftp"},
        {22,   "ssh"},
        {23,   "telnet"},
        {80,   "http"},
        {443,  "https"},
        {515,  "lpd"},
        {631,  "ipp"},
        {139,  "smb"},
        {445,  "smb"},
        {9100, "raw"},
    };

    for (auto& ch : checks) {
        const bool open = tcp_port_open(ip, ch.port);
        if (open) {
            s.open_ports.push_back(ch.port);
            if (ch.name == "http")   s.http_enabled   = true;
            if (ch.name == "https")  s.https_enabled  = true;
            if (ch.name == "telnet") s.telnet_enabled = true;
            if (ch.name == "ftp")    s.ftp_enabled    = true;
            if (ch.name == "ssh")    s.ssh_enabled    = true;
            if (ch.name == "smb")    s.smb_enabled    = true;
        }
    }

    s.default_password = false;
    s.collected_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return s;
}
