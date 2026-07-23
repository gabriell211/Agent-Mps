#pragma once

#include "Discovery.hpp"
#include <functional>

class ArpScanner {
public:
    std::vector<PrinterEndpoint> scan(
        const std::function<bool(const std::string&)>& is_printer,
        const std::function<std::string(const std::string&)>& resolve_hostname) const;
};
