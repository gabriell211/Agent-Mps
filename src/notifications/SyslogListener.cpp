#include "SyslogListener.hpp"
#include "../core/Logger.hpp"

#include <array>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

void SyslogListener::start(std::function<void(const std::string&, const std::string&)> handler) {
    if (!cfg.enabled || running) return;
    callback = std::move(handler);
    running = true;
    worker = std::thread([this] {
#ifdef _WIN32
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) { running = false; return; }
#endif
#ifdef _WIN32
        const SOCKET socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_fd == INVALID_SOCKET) { WSACleanup(); running = false; return; }
#else
        const int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket_fd < 0) { running = false; return; }
#endif
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<uint16_t>(cfg.port));
        if (inet_pton(AF_INET, cfg.host.c_str(), &address.sin_addr) != 1 ||
            bind(socket_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
            Log::error("syslog: nao foi possivel escutar em {}:{}", cfg.host, cfg.port);
#ifdef _WIN32
            closesocket(socket_fd); WSACleanup();
#else
            close(socket_fd);
#endif
            running = false;
            return;
        }
        while (running) {
            fd_set read_set;
            FD_ZERO(&read_set); FD_SET(socket_fd, &read_set);
            timeval wait{1, 0};
#ifdef _WIN32
            if (select(0, &read_set, nullptr, nullptr, &wait) <= 0) continue;
#else
            if (select(socket_fd + 1, &read_set, nullptr, nullptr, &wait) <= 0) continue;
#endif
            std::array<char, 4096> buffer{};
            sockaddr_in sender{};
#ifdef _WIN32
            int size = sizeof(sender);
#else
            socklen_t size = sizeof(sender);
#endif
            const int received = recvfrom(socket_fd, buffer.data(), static_cast<int>(buffer.size() - 1), 0,
                                          reinterpret_cast<sockaddr*>(&sender), &size);
            if (received <= 0) continue;
            char source[INET_ADDRSTRLEN]{};
            inet_ntop(AF_INET, &sender.sin_addr, source, sizeof(source));
            const std::string message(buffer.data(), static_cast<size_t>(received));
            if (callback) callback(source, message);
            else Log::info("syslog [{}]: {}", source, message);
        }
#ifdef _WIN32
        closesocket(socket_fd); WSACleanup();
#else
        close(socket_fd);
#endif
    });
}

void SyslogListener::stop() {
    running = false;
    if (worker.joinable()) worker.join();
}
