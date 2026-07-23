#pragma once

#include "Discovery.hpp"

class MdnsDiscovery {
public:
    std::vector<PrinterEndpoint> scan(int timeout_ms) const;
};
