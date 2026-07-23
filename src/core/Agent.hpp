#pragma once
#include "Config.hpp"
#include "Scheduler.hpp"
#include "ThreadPool.hpp"
#include "../storage/Database.hpp"
#include "../storage/Repository.hpp"
#include "../collectors/SnmpCollector.hpp"
#include "../collectors/IppCollector.hpp"
#include "../collectors/HttpCollector.hpp"
#include "../collectors/UsbCollector.hpp"
#include "../discovery/Discovery.hpp"
#include "../modules/Inventory.hpp"
#include "../modules/Status.hpp"
#include "../modules/Consumables.hpp"
#include "../modules/Counters.hpp"
#include "../modules/NetworkInfo.hpp"
#include "../modules/Security.hpp"
#include "../modules/Jobs.hpp"
#include "../notifications/AlertManager.hpp"
#include "../notifications/WebhookNotifier.hpp"
#include "../notifications/EmailNotifier.hpp"
#include "../notifications/SyslogListener.hpp"
#include "../api/RestServer.hpp"
#include <memory>
#include <atomic>
#include <mutex>

class Agent {
public:
    Agent();
    ~Agent() = default;

    bool init(const std::string& config_path);
    void run();    // bloqueia ate receber SIGTERM/SIGINT
    void stop();

private:
    AgentConfig cfg;
    Database    db;
    std::unique_ptr<Repository>      repo;
    std::unique_ptr<SnmpCollector>   snmp;
    std::unique_ptr<IppCollector>    ipp;
    std::unique_ptr<HttpCollector>   http;
    std::unique_ptr<UsbCollector>    usb;
    std::unique_ptr<AlertManager>    alerts;
    std::unique_ptr<WebhookNotifier> webhook;
    std::unique_ptr<EmailNotifier>   email;
    std::unique_ptr<SyslogListener>  syslog;
    std::unique_ptr<InventoryModule> inventory_module;
    std::unique_ptr<StatusModule>    status_module;
    std::unique_ptr<ConsumablesModule> consumables_module;
    std::unique_ptr<CountersModule>  counters_module;
    std::unique_ptr<NetworkInfoModule> network_module;
    std::unique_ptr<SecurityModule> security_module;
    std::unique_ptr<JobsModule> jobs_module;
    std::unique_ptr<RestServer>      api;
    std::unique_ptr<Scheduler>       scheduler;
    std::unique_ptr<ThreadPool>      pool;
    std::atomic<bool>                stop_flag{false};
    std::mutex discovery_mu;
    std::mutex collection_mu;

    // ciclos principais
    void do_discovery();
    void do_collection();

    // coleta completa de uma impressora
    void collect_printer(Printer& p);
    void collect_usb_printer(Printer& p, long long now);
};
