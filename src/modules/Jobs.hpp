#pragma once

#include "../models/NetworkConfig.hpp"
#include "../models/Printer.hpp"
#include <vector>

class JobsModule {
public:
    std::vector<PrintJob> collect(const Printer& printer) const;
};
