#pragma once
#include "../collectors/SnmpCollector.hpp"
#include "../core/Config.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

struct PrinterEndpoint {
    std::string ip;
    std::string hostname;
    std::string mac;
    int         response_ms = 0;
    int         snmp_port   = 161;
    bool        has_snmp    = false;
    bool        has_ipp     = false;
    bool        has_http    = false;
};

class Discovery {
public:
    Discovery(const DiscoveryConfig& cfg, const SnmpConfig& snmp_cfg);

    std::vector<PrinterEndpoint> discover_all();
    std::vector<PrinterEndpoint> scan_ip_ranges();
    std::vector<PrinterEndpoint> scan_arp();
    std::vector<PrinterEndpoint> scan_mdns();

    void set_on_found(std::function<void(const PrinterEndpoint&)> cb) { on_found = cb; }

private:
    const DiscoveryConfig& cfg;
    std::unique_ptr<SnmpCollector> snmp_probe;
    std::function<void(const PrinterEndpoint&)> on_found;

    void deduplicate(std::vector<PrinterEndpoint>& list);
    bool probe_snmp(const std::string& ip, int port, int timeout_ms);
    bool probe_tcp(const std::string& ip, int port, int timeout_ms);
    std::string resolve_hostname(const std::string& ip);
};
