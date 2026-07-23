#pragma once
#include "../models/Printer.hpp"
#include "../models/Consumable.hpp"
#include <string>
#include <vector>
#include <map>

class HttpCollector {
public:
    HttpCollector() = default;

    std::string fetch(const std::string& url, int timeout_s = 5);

    Printer        collect_inventory_ews(const std::string& ip, const std::string& manufacturer);

    std::string    parse_hp_ews_status(const std::string& body);

    static std::string xml_value(const std::string& body, const std::string& tag);

    static std::vector<std::string> xml_values(const std::string& body, const std::string& tag);

private:
    std::string get_http(const std::string& url, int timeout_s);
};
