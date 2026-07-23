#include "MdnsDiscovery.hpp"
#include "../core/Logger.hpp"

#include <array>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <set>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {
void put_name(std::vector<unsigned char>& out, const char* name) {
    const char* start = name;
    while (*start) {
        const char* end = std::strchr(start, '.');
        const size_t length = end ? static_cast<size_t>(end - start) : std::strlen(start);
        out.push_back(static_cast<unsigned char>(length));
        out.insert(out.end(), start, start + length);
        if (!end) break;
        start = end + 1;
    }
    out.push_back(0);
}

std::vector<unsigned char> query(const char* service) {
    std::vector<unsigned char> packet = {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0};
    put_name(packet, service);
    packet.insert(packet.end(), {0, 12, 0, 1});
    return packet;
}
}

std::vector<PrinterEndpoint> MdnsDiscovery::scan(int timeout_ms) const {
    std::vector<PrinterEndpoint> result;
#ifdef _WIN32
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return result;
#endif
#ifdef _WIN32
    const SOCKET socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd == INVALID_SOCKET) { WSACleanup(); return result; }
#else
    const int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) return result;
#endif
    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_port = htons(5353);
    inet_pton(AF_INET, "224.0.0.251", &target.sin_addr);
    for (const char* service : {"_ipp._tcp.local", "_ipps._tcp.local", "_printer._tcp.local"}) {
        const auto packet = query(service);
        sendto(socket_fd, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()), 0,
               reinterpret_cast<sockaddr*>(&target), sizeof(target));
    }

    std::set<std::string> addresses;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(std::max(100, timeout_ms));
    while (std::chrono::steady_clock::now() < deadline) {
        fd_set read_set;
        FD_ZERO(&read_set);
        FD_SET(socket_fd, &read_set);
        timeval wait{0, 100000};
#ifdef _WIN32
        if (select(0, &read_set, nullptr, nullptr, &wait) <= 0) continue;
#else
        if (select(socket_fd + 1, &read_set, nullptr, nullptr, &wait) <= 0) continue;
#endif
        std::array<unsigned char, 1500> packet{};
        sockaddr_in sender{};
#ifdef _WIN32
        int sender_size = sizeof(sender);
#else
        socklen_t sender_size = sizeof(sender);
#endif
        const int received = recvfrom(socket_fd, reinterpret_cast<char*>(packet.data()), static_cast<int>(packet.size()), 0,
                                      reinterpret_cast<sockaddr*>(&sender), &sender_size);
        if (received < 12) continue;
        for (int i = 0; i + 15 < received; ++i) {
            if (packet[static_cast<size_t>(i)] == 0 && packet[static_cast<size_t>(i + 1)] == 1 &&
                packet[static_cast<size_t>(i + 2)] == 0 && packet[static_cast<size_t>(i + 3)] == 1 &&
                packet[static_cast<size_t>(i + 8)] == 0 && packet[static_cast<size_t>(i + 9)] == 4) {
                char ip[INET_ADDRSTRLEN]{};
                in_addr address{};
                std::memcpy(&address, packet.data() + i + 10, 4);
                if (inet_ntop(AF_INET, &address, ip, sizeof(ip))) addresses.insert(ip);
            }
        }
    }
#ifdef _WIN32
    closesocket(socket_fd);
    WSACleanup();
#else
    close(socket_fd);
#endif
    for (const auto& ip : addresses) {
        PrinterEndpoint endpoint;
        endpoint.ip = ip;
        endpoint.has_ipp = true;
        result.push_back(std::move(endpoint));
    }
    return result;
}
