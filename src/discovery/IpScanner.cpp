#include "IpScanner.hpp"
#include "../core/Logger.hpp"
#include "../core/ThreadPool.hpp"

#include <algorithm>
#include <mutex>
#include <stdexcept>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace {
std::vector<std::string> expand_cidr(const std::string& cidr) {
    const auto slash = cidr.find('/');
    if (slash == std::string::npos) return {cidr};

    const int prefix = std::stoi(cidr.substr(slash + 1));
    if (prefix < 8 || prefix > 30) {
        throw std::invalid_argument("CIDR deve estar entre /8 e /30: " + cidr);
    }
    in_addr address{};
    if (inet_pton(AF_INET, cidr.substr(0, slash).c_str(), &address) != 1) {
        throw std::invalid_argument("endereco CIDR invalido: " + cidr);
    }
    const uint32_t mask = ~uint32_t{0} << (32 - prefix);
    const uint32_t base = ntohl(address.s_addr) & mask;
    const uint32_t last = base | ~mask;
    std::vector<std::string> result;
    result.reserve(static_cast<size_t>(last - base - 1));
    for (uint32_t value = base + 1; value < last; ++value) {
        in_addr current{};
        current.s_addr = htonl(value);
        char text[INET_ADDRSTRLEN]{};
        inet_ntop(AF_INET, &current, text, sizeof(text));
        result.emplace_back(text);
    }
    return result;
}
}

std::vector<PrinterEndpoint> IpScanner::scan(
    const std::function<PrinterEndpoint(const std::string&)>& inspect) const {
    std::vector<std::string> addresses;
    for (const auto& range : cfg.ip_ranges) {
        try {
            auto expanded = expand_cidr(range);
            addresses.insert(addresses.end(), expanded.begin(), expanded.end());
        } catch (const std::exception& error) {
            Log::warn("discovery: ignorando faixa '{}': {}", range, error.what());
        }
    }

    std::vector<PrinterEndpoint> found;
    std::mutex found_mutex;
    ThreadPool pool(std::max(1, cfg.threads));
    for (const auto& ip : addresses) {
        pool.submit([&inspect, &found, &found_mutex, ip] {
            auto endpoint = inspect(ip);
            if (endpoint.ip.empty()) return;
            std::lock_guard<std::mutex> lock(found_mutex);
            found.push_back(std::move(endpoint));
        });
    }
    pool.wait();
    return found;
}
