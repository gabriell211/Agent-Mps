#pragma once
#include "../models/Printer.hpp"
#include "../models/Consumable.hpp"
#include "../models/NetworkConfig.hpp"
#include <string>
#include <vector>
#include <map>
#include <cstdint>

class IppCollector {
public:
    IppCollector() = default;

    bool check(const std::string& ip, int port = 631);

    PrinterStatus collect_status(const std::string& ip, int printer_id, int port = 631);

    std::vector<Consumable> collect_consumables(const std::string& ip, int printer_id, int port = 631);

private:
    std::vector<uint8_t> build_get_printer_attrs_request(const std::vector<std::string>& attrs,
                                                         const std::string& printer_uri);

    std::map<std::string, std::string> parse_ipp_response(const std::vector<uint8_t>& data);

    std::vector<uint8_t> send_ipp(const std::string& ip, int port, const std::vector<uint8_t>& payload);
};
