#include "SnmpDiscovery.hpp"

std::vector<PrinterEndpoint> SnmpDiscovery::discover(
    const std::vector<std::string>& addresses,
    const std::function<bool(const std::string&)>& probe,
    const std::function<std::string(const std::string&)>& resolve_hostname) const {
    std::vector<PrinterEndpoint> result;
    for (const auto& ip : addresses) {
        if (!probe(ip)) continue;
        PrinterEndpoint endpoint;
        endpoint.ip = ip;
        endpoint.has_snmp = true;
        endpoint.hostname = resolve_hostname(ip);
        result.push_back(std::move(endpoint));
    }
    return result;
}
