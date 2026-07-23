#pragma once
#include "../models/Printer.hpp"
#include "../models/Consumable.hpp"
#include "../models/Counter.hpp"
#include "../models/NetworkConfig.hpp"
#include "../core/Config.hpp"
#include <string>
#include <vector>
#include <optional>

class SnmpCollector {
public:
    explicit SnmpCollector(const SnmpConfig& cfg);
    ~SnmpCollector();

    int ping(const std::string& ip);

    Printer       collect_inventory(const std::string& ip);
    PrinterStatus collect_status(const std::string& ip, int printer_id);
    std::vector<Consumable> collect_consumables(const std::string& ip, int printer_id);
    Counter collect_counters(const std::string& ip, int printer_id);
    NetworkConfig collect_network(const std::string& ip, int printer_id);
    SecurityInfo  collect_security(const std::string& ip, int printer_id);

private:
    SnmpConfig cfg;
    std::string get_oid(const std::string& ip, const std::string& oid);
    long long   get_oid_int(const std::string& ip, const std::string& oid);
    std::vector<std::pair<std::string,std::string>>
                walk_oid(const std::string& ip, const std::string& base_oid);
    std::string detect_manufacturer(const std::string& sys_oid);
    void apply_vendor_oids(const std::string& ip, Printer& p, const std::string& mfr);
    void apply_vendor_consumables(const std::string& ip, std::vector<Consumable>& out,
                                  int printer_id, const std::string& mfr);
};
