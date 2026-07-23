#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static bool parseBool(const std::string& v) {
    return (v == "true" || v == "1" || v == "yes");
}

static std::vector<std::string> parseList(const std::string& raw) {
    std::vector<std::string> out;
    auto start = raw.find('[');
    auto end   = raw.rfind(']');
    if (start == std::string::npos || end == std::string::npos) return out;
    std::string inner = raw.substr(start + 1, end - start - 1);
    std::stringstream ss(inner);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        tok = trim(tok);
        if (!tok.empty() && tok.front() == '"') tok = tok.substr(1);
        if (!tok.empty() && tok.back()  == '"') tok.pop_back();
        tok = trim(tok);
        if (!tok.empty()) out.push_back(tok);
    }
    return out;
}

static std::string stripQuotes(const std::string& v) {
    if (v.size() >= 2 && v.front() == '"' && v.back() == '"')
        return v.substr(1, v.size() - 2);
    return v;
}

bool Config::load(const std::string& path) {
    cfg = AgentConfig{};
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[config] nao encontrou " << path << ", usando defaults\n";
        return false;
    }

    try {
        std::string section;
        std::string line;
        while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            section = trim(section);
            continue;
        }

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = trim(line.substr(0, eq));
        std::string val = trim(line.substr(eq + 1));
        val = stripQuotes(val);

        if (section == "agent") {
            if (key == "name")                cfg.name = val;
            else if (key == "collection_interval") cfg.collection_interval = std::stoi(val);
            else if (key == "discovery_interval")  cfg.discovery_interval  = std::stoi(val);
            else if (key == "thread_pool_size")    cfg.thread_pool_size    = std::stoi(val);
            else if (key == "log_level")           cfg.log_level  = val;
            else if (key == "log_file")            cfg.log_file   = val;
        }
        else if (section == "snmp") {
            if (key == "version")    cfg.snmp.version   = val;
            else if (key == "community")  cfg.snmp.community = val;
            else if (key == "timeout")   cfg.snmp.timeout   = std::stoi(val);
            else if (key == "retries")   cfg.snmp.retries   = std::stoi(val);
            else if (key == "port")      cfg.snmp.port      = std::stoi(val);
            else if (key == "auth_proto")cfg.snmp.auth_proto = val;
            else if (key == "priv_proto")cfg.snmp.priv_proto = val;
            else if (key == "auth_pass") cfg.snmp.auth_pass  = val;
            else if (key == "priv_pass") cfg.snmp.priv_pass  = val;
            else if (key == "sec_name")  cfg.snmp.sec_name   = val;
        }
        else if (section == "discovery") {
            if (key == "ip_ranges")  cfg.discovery.ip_ranges = parseList(line.substr(eq+1));
            else if (key == "ports") {
                auto tmp = parseList(line.substr(eq+1));
                cfg.discovery.ports.clear();
                for (auto& p : tmp) cfg.discovery.ports.push_back(std::stoi(p));
            }
            else if (key == "mdns")    cfg.discovery.mdns    = parseBool(val);
            else if (key == "arp")     cfg.discovery.arp     = parseBool(val);
            else if (key == "usb")     cfg.discovery.usb     = parseBool(val);
            else if (key == "threads") cfg.discovery.threads = std::stoi(val);
            else if (key == "timeout_ms") cfg.discovery.timeout_ms = std::stoi(val);
        }
        else if (section == "database") {
            if (key == "path") cfg.database.path = val;
        }
        else if (section == "api") {
            if (key == "host")  cfg.api.host  = val;
            else if (key == "port")  cfg.api.port  = std::stoi(val);
            else if (key == "token") cfg.api.token = val;
        }
        else if (section == "alerts") {
            if (key == "toner_warning")   cfg.alert_thresholds.toner_warning  = std::stoi(val);
            else if (key == "toner_critical")  cfg.alert_thresholds.toner_critical = std::stoi(val);
            else if (key == "offline_minutes") cfg.alert_thresholds.offline_minutes = std::stoi(val);
        }
        else if (section == "notifications.webhook") {
            if (key == "url")     cfg.webhook.url     = val;
            else if (key == "enabled") cfg.webhook.enabled = parseBool(val);
        }
        else if (section == "notifications.smtp") {
            if (key == "host")    cfg.smtp.host    = val;
            else if (key == "port")    cfg.smtp.port    = std::stoi(val);
            else if (key == "from")    cfg.smtp.from    = val;
            else if (key == "to")      cfg.smtp.to      = parseList(line.substr(eq+1));
            else if (key == "user")    cfg.smtp.user    = val;
            else if (key == "pass")    cfg.smtp.pass    = val;
            else if (key == "enabled") cfg.smtp.enabled = parseBool(val);
        }
        else if (section == "notifications.syslog") {
            if (key == "host") cfg.syslog.host = val;
            else if (key == "port") cfg.syslog.port = std::stoi(val);
            else if (key == "enabled") cfg.syslog.enabled = parseBool(val);
        }
        }
    } catch (const std::exception& error) {
        std::cerr << "[config] erro em " << path << ": " << error.what() << "\n";
        return false;
    }
    return true;
}
