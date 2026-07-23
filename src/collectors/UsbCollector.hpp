#pragma once

#include "../models/Printer.hpp"
#include <vector>
class UsbCollector {
public:
    std::vector<Printer> discover() const;
    PrinterStatus collect_status(const Printer& printer) const;
};
