#include "ArpScanner.hpp"
#include "../core/Logger.hpp"

#include <cstdio>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

std::vector<PrinterEndpoint> ArpScanner::scan(
    const std::function<bool(const std::string&)>& is_printer,
    const std::function<std::string(const std::string&)>& resolve_hostname) const {
    std::vector<PrinterEndpoint> result;
#ifdef _WIN32
    PMIB_IPNET_TABLE2 table = nullptr;
    if (GetIpNetTable2(AF_INET, &table) != NO_ERROR || !table) return result;
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const auto& row = table->Table[i];
        if (row.PhysicalAddressLength == 0) continue;
        char ip[INET_ADDRSTRLEN]{};
        if (!InetNtopA(AF_INET, &row.Address.Ipv4.sin_addr, ip, sizeof(ip))) continue;
        if (!is_printer(ip)) continue;
        PrinterEndpoint endpoint;
        endpoint.ip = ip;
        endpoint.has_snmp = true;
        endpoint.hostname = resolve_hostname(ip);
        std::ostringstream mac;
        for (ULONG j = 0; j < row.PhysicalAddressLength; ++j) {
            if (j) mac << ':';
            mac << std::hex << std::uppercase << static_cast<int>(row.PhysicalAddress[j]);
        }
        endpoint.mac = mac.str();
        result.push_back(std::move(endpoint));
    }
    FreeMibTable(table);
#else
    FILE* file = std::fopen("/proc/net/arp", "r");
    if (!file) return result;
    char line[256]{};
    std::fgets(line, sizeof(line), file);
    while (std::fgets(line, sizeof(line), file)) {
        char ip[64]{}, hw[64]{}, mac[64]{}, dev[64]{};
        unsigned int type = 0, flags = 0;
        if (std::sscanf(line, "%63s 0x%x 0x%x %63s %63s %63s", ip, &type, &flags, mac, hw, dev) < 4) continue;
        if (!(flags & 0x02) || !is_printer(ip)) continue;
        PrinterEndpoint endpoint;
        endpoint.ip = ip;
        endpoint.mac = mac;
        endpoint.has_snmp = true;
        endpoint.hostname = resolve_hostname(ip);
        result.push_back(std::move(endpoint));
    }
    std::fclose(file);
#endif
    return result;
}
