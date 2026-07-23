#pragma once

#include "Discovery.hpp"
#include <functional>

class IpScanner {
public:
    explicit IpScanner(const DiscoveryConfig& cfg) : cfg(cfg) {}
    std::vector<PrinterEndpoint> scan(
        const std::function<PrinterEndpoint(const std::string&)>& inspect) const;

private:
    const DiscoveryConfig& cfg;
};
