#pragma once

#include "Discovery.hpp"
#include <functional>

class SnmpDiscovery {
public:
    std::vector<PrinterEndpoint> discover(const std::vector<std::string>& addresses,
        const std::function<bool(const std::string&)>& probe,
        const std::function<std::string(const std::string&)>& resolve_hostname) const;
};
