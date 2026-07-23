#pragma once
#include "../storage/Repository.hpp"
#include "../collectors/SnmpCollector.hpp"
#include "../collectors/HttpCollector.hpp"
#include "../models/Alert.hpp"
#include <string>
#include <vector>
#include <functional>
#include <mutex>

class AlertManager {
public:
    AlertManager(Repository& repo, const AlertThresholds& thresholds);

    void check_consumables(int printer_id, const std::vector<Consumable>& consumables);

    void check_offline(int printer_id, bool is_online, long long last_seen = 0, long long now = 0);

    void check_status(int printer_id, const PrinterStatus& status);

    void check_firmware_change(int printer_id, const std::string& old_fw, const std::string& new_fw);

    void resolve_by_type(int printer_id, const std::string& type);

    void set_on_raised(std::function<void(const Alert&)> callback);

private:
    Repository&        repo;
    AlertThresholds    thresholds;
    std::function<void(const Alert&)> on_raised;
    std::mutex mu;

    bool has_open_alert(int printer_id, const std::string& type);
    int  raise_alert(int printer_id, const std::string& severity,
                     const std::string& type, const std::string& message);
};
