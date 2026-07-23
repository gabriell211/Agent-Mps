#pragma once

#include <string>

struct PrinterEvent {
    int id = 0;
    int printer_id = 0;
    std::string type;
    std::string message;
    long long created_at = 0;
};

struct PrinterLog {
    int id = 0;
    int printer_id = 0;
    std::string source;
    std::string message;
    long long created_at = 0;
};
