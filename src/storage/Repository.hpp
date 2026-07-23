#pragma once
#include "Database.hpp"
#include "../models/Printer.hpp"
#include "../models/Consumable.hpp"
#include "../models/Counter.hpp"
#include "../models/Alert.hpp"
#include "../models/NetworkConfig.hpp"
#include "../models/Event.hpp"
#include <vector>
#include <optional>
#include <mutex>

class Repository {
public:
    explicit Repository(Database& db) : db(db) {}

    int         save_printer(const Printer& p);      // insert ou update por ip
    void        update_last_seen(int id, long long ts, int rtt_ms);
    std::optional<Printer> find_printer(int id);
    std::optional<Printer>    find_printer_by_ip(const std::string& ip);
    std::vector<Printer>      all_printers();
    void        mark_inactive(int id);

    void save_status(const PrinterStatus& s);
    std::optional<PrinterStatus> latest_status(int printer_id);

    void save_consumables(const std::vector<Consumable>& items);
    std::vector<Consumable> latest_consumables(int printer_id);

    void save_counter(const Counter& c);
    std::optional<Counter> latest_counter(int printer_id);

    void save_network(const NetworkConfig& n);
    std::optional<NetworkConfig> latest_network(int printer_id);

    void save_security(const SecurityInfo& s);
    std::optional<SecurityInfo> latest_security(int printer_id);

    int  save_alert(const Alert& a);
    void resolve_alert(int id, long long ts);
    std::vector<Alert> open_alerts(int printer_id);
    std::vector<Alert> all_open_alerts();

    void save_jobs(const std::vector<PrintJob>& jobs);
    std::vector<PrintJob> latest_jobs(int printer_id, int limit = 20);

    void save_event(int printer_id, const std::string& type, const std::string& message, long long ts = 0);
    void save_log(int printer_id, const std::string& source, const std::string& message, long long ts = 0);
    std::vector<PrinterEvent> latest_events(int printer_id, int limit = 100);
    std::vector<PrinterLog> latest_logs(int printer_id, int limit = 100);

    void record_history(int printer_id, const std::string& metric, double value, long long ts);
    std::vector<std::pair<long long,double>> history(int printer_id, const std::string& metric, int limit = 100);

private:
    Database& db;
    std::recursive_mutex mu;
};
