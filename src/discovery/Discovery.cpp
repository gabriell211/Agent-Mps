#include "Discovery.hpp"
#include "IpScanner.hpp"
#include "ArpScanner.hpp"
#include "MdnsDiscovery.hpp"
#include "../core/Config.hpp"
#include "../core/Logger.hpp"
#include "../core/ThreadPool.hpp"
#include <mutex>
#include <set>
#include <cstring>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <fcntl.h>
  #include <errno.h>
  #include <unistd.h>
  #include <net-snmp/net-snmp-config.h>
  #include <net-snmp/net-snmp-includes.h>
#endif

Discovery::Discovery(const DiscoveryConfig& cfg, const SnmpConfig& snmp_cfg)
    : cfg(cfg), snmp_probe(std::make_unique<SnmpCollector>(snmp_cfg)) {
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        Log::warn("discovery: WSAStartup falhou");
    }
#endif
}

bool Discovery::probe_tcp(const std::string& ip, int port, int timeout_ms) {
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return false;
    u_long nonblocking = 1;
    ioctlsocket(sock, FIONBIO, &nonblocking);
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    bool ok = (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);
    if (!ok && (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINPROGRESS)) {
        fd_set writes;
        FD_ZERO(&writes); FD_SET(sock, &writes);
        timeval timeout{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
        if (select(0, nullptr, &writes, nullptr, &timeout) > 0) {
            int error = 0, size = sizeof(error);
            ok = getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &size) == 0 && error == 0;
        }
    }
    closesocket(sock);
    return ok;
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    const int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    bool ok = (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0);
    if (!ok && errno == EINPROGRESS) {
        fd_set writes;
        FD_ZERO(&writes); FD_SET(sock, &writes);
        timeval timeout{timeout_ms / 1000, (timeout_ms % 1000) * 1000};
        if (select(sock + 1, nullptr, &writes, nullptr, &timeout) > 0) {
            int error = 0;
            socklen_t size = sizeof(error);
            ok = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &size) == 0 && error == 0;
        }
    }
    ::close(sock);
    return ok;
#endif
}

bool Discovery::probe_snmp(const std::string& ip, int port, int timeout_ms) {
    (void)port;
    (void)timeout_ms;
    return snmp_probe && snmp_probe->ping(ip) >= 0;
}

std::string Discovery::resolve_hostname(const std::string& ip) {
    struct sockaddr_in sa = {};
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);
    char host[NI_MAXHOST] = {};
    int rc = getnameinfo((sockaddr*)&sa, sizeof(sa), host, sizeof(host), nullptr, 0, NI_NAMEREQD);
    return (rc == 0) ? host : "";
}

static std::vector<std::string> expand_cidr(const std::string& cidr) {
    std::vector<std::string> ips;
    auto slash = cidr.find('/');
    if (slash == std::string::npos) { ips.push_back(cidr); return ips; }

    std::string net_str = cidr.substr(0, slash);
    int prefix = std::stoi(cidr.substr(slash + 1));

    struct in_addr net_addr;
    inet_pton(AF_INET, net_str.c_str(), &net_addr);
    uint32_t net  = ntohl(net_addr.s_addr);
    uint32_t mask = prefix == 0 ? 0 : (~0u << (32 - prefix));
    uint32_t base = net & mask;
    uint32_t top  = base | (~mask);

    for (uint32_t ip = base + 1; ip < top; ip++) {
        struct in_addr a;
        a.s_addr = htonl(ip);
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &a, buf, sizeof(buf));
        ips.push_back(buf);
    }
    return ips;
}

std::vector<PrinterEndpoint> Discovery::scan_ip_ranges() {
    IpScanner scanner(cfg);
    return scanner.scan([this](const std::string& ip) {
        const bool snmp_ok = probe_snmp(ip, 161, cfg.timeout_ms);
        const bool ipp_ok = std::find(cfg.ports.begin(), cfg.ports.end(), 631) != cfg.ports.end()
            && probe_tcp(ip, 631, cfg.timeout_ms / 2);
        const bool raw_ok = std::find(cfg.ports.begin(), cfg.ports.end(), 9100) != cfg.ports.end()
            && probe_tcp(ip, 9100, cfg.timeout_ms / 2);
        if (!snmp_ok && !ipp_ok && !raw_ok) return PrinterEndpoint{};
        PrinterEndpoint endpoint;
        endpoint.ip = ip;
        endpoint.has_snmp = snmp_ok;
        endpoint.has_ipp = ipp_ok;
        endpoint.has_http = (std::find(cfg.ports.begin(), cfg.ports.end(), 80) != cfg.ports.end() &&
                             probe_tcp(ip, 80, cfg.timeout_ms / 2)) ||
                            (std::find(cfg.ports.begin(), cfg.ports.end(), 443) != cfg.ports.end() &&
                             probe_tcp(ip, 443, cfg.timeout_ms / 2));
        endpoint.hostname = resolve_hostname(ip);
        return endpoint;
    });
}

std::vector<PrinterEndpoint> Discovery::scan_arp() {
    ArpScanner scanner;
    return scanner.scan([this](const std::string& ip) {
        return probe_snmp(ip, 161, cfg.timeout_ms);
    }, [this](const std::string& ip) { return resolve_hostname(ip); });
}

std::vector<PrinterEndpoint> Discovery::scan_mdns() {
    MdnsDiscovery scanner;
    return scanner.scan(cfg.timeout_ms);
}

std::vector<PrinterEndpoint> Discovery::discover_all() {
    std::vector<PrinterEndpoint> all;

    auto from_scan = scan_ip_ranges();
    all.insert(all.end(), from_scan.begin(), from_scan.end());

    if (cfg.arp) {
        auto from_arp = scan_arp();
        all.insert(all.end(), from_arp.begin(), from_arp.end());
    }
    if (cfg.mdns) {
        auto from_mdns = scan_mdns();
        all.insert(all.end(), from_mdns.begin(), from_mdns.end());
    }

    deduplicate(all);
    if (on_found) {
        for (const auto& endpoint : all) on_found(endpoint);
    }
    Log::info("discovery: {} dispositivos encontrados no total", all.size());
    return all;
}

void Discovery::deduplicate(std::vector<PrinterEndpoint>& list) {
    std::set<std::string> seen;
    std::vector<PrinterEndpoint> result;
    for (auto& ep : list) {
        if (seen.insert(ep.ip).second)
            result.push_back(ep);
    }
    list = std::move(result);
}
