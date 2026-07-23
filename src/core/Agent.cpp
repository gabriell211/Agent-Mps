#include "Agent.hpp"
#include "Logger.hpp"
#include "../storage/Migrations.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <mutex>

Agent::Agent() {}

bool Agent::init(const std::string& config_path) {
    if (!Config::get().load(config_path)) {
        std::cerr << "configuracao obrigatoria ausente ou invalida: " << config_path << "\n";
        return false;
    }
    cfg = Config::get().data();

    Log::init(cfg.log_level, cfg.log_file);
    Log::info("santa-agent iniciando...");
    if (cfg.api.token.size() < 32) {
        Log::error("api.token deve ter pelo menos 32 caracteres");
        return false;
    }
    if (cfg.collection_interval < 30 || cfg.discovery_interval < 60 || cfg.thread_pool_size < 1 ||
        cfg.discovery.threads < 1 || cfg.discovery.timeout_ms < 100 ||
        cfg.api.port < 1 || cfg.api.port > 65535) {
        Log::error("intervalos ou thread_pool_size invalidos");
        return false;
    }

    try {
        db.open(cfg.database.path);
        Migrations::run(db);
    } catch (const std::exception& e) {
        Log::error("falha ao abrir banco: {}", e.what());
        return false;
    }

    repo    = std::make_unique<Repository>(db);
    snmp    = std::make_unique<SnmpCollector>(cfg.snmp);
    ipp     = std::make_unique<IppCollector>();
    http    = std::make_unique<HttpCollector>();
    usb     = std::make_unique<UsbCollector>();
    alerts  = std::make_unique<AlertManager>(*repo, cfg.alert_thresholds);
    webhook = std::make_unique<WebhookNotifier>(cfg.webhook);
    email   = std::make_unique<EmailNotifier>(cfg.smtp);
    syslog  = std::make_unique<SyslogListener>(cfg.syslog);
    inventory_module = std::make_unique<InventoryModule>(*snmp, *http);
    status_module = std::make_unique<StatusModule>(*snmp, *ipp);
    consumables_module = std::make_unique<ConsumablesModule>(*snmp, *ipp);
    counters_module = std::make_unique<CountersModule>(*snmp);
    network_module = std::make_unique<NetworkInfoModule>(*snmp);
    security_module = std::make_unique<SecurityModule>(*snmp);
    jobs_module = std::make_unique<JobsModule>();
    pool    = std::make_unique<ThreadPool>(cfg.thread_pool_size);
    scheduler = std::make_unique<Scheduler>();
    api     = std::make_unique<RestServer>(*repo, cfg.api, [this] {
        if (pool) pool->submit([this] { do_discovery(); });
    });
    alerts->set_on_raised([this](const Alert& alert) {
        const auto printer = repo->find_printer(alert.printer_id);
        if (!printer) return;
        webhook->notify(alert, *printer);
        email->notify(alert, *printer);
    });

    // agenda coleta periodica
    scheduler->add("coleta", [this] { do_collection(); }, cfg.collection_interval, true);

    // agenda descoberta periodica
    scheduler->add("discovery", [this] { do_discovery(); }, cfg.discovery_interval, true);

    Log::info("agente iniciado. coleta a cada {}s, discovery a cada {}s",
              cfg.collection_interval, cfg.discovery_interval);
    return true;
}

void Agent::run() {
    api->start();
    syslog->start([this](const std::string& source, const std::string& message) {
        const auto printer = repo->find_printer_by_ip(source);
        const int printer_id = printer ? printer->id : 0;
        repo->save_log(printer_id, source, message);
        if (printer) repo->save_event(printer_id, "syslog", message);
    });
    scheduler->start();

    while (!stop_flag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    scheduler->stop();
    api->stop();
    syslog->stop();
    Log::info("agente encerrado.");
}

void Agent::stop() {
    stop_flag = true;
}

void Agent::do_discovery() {
    std::unique_lock<std::mutex> cycle_lock(discovery_mu, std::try_to_lock);
    if (!cycle_lock.owns_lock()) {
        Log::warn("discovery: ciclo anterior ainda em execucao; ignorando solicitacao duplicada");
        return;
    }
    Log::info("discovery: iniciando varredura");
    Discovery disc(cfg.discovery, cfg.snmp);
    disc.set_on_found([this](const PrinterEndpoint& ep) {
        Printer p = inventory_module->collect(ep);
        int id = repo->save_printer(p);
        Log::info("discovery: impressora salva ip={} modelo='{}' id={}", p.ip, p.model, id);
    });

    disc.discover_all();
    if (cfg.discovery.usb) {
        for (const auto& printer : usb->discover()) {
            const int id = repo->save_printer(printer);
            Log::info("discovery: impressora USB salva porta={} id={}", printer.port_name, id);
        }
    }
}

void Agent::do_collection() {
    std::unique_lock<std::mutex> cycle_lock(collection_mu, std::try_to_lock);
    if (!cycle_lock.owns_lock()) {
        Log::warn("coleta: ciclo anterior ainda em execucao; ignorando solicitacao duplicada");
        return;
    }
    auto printers = repo->all_printers();
    Log::info("coleta: {} impressoras ativas", printers.size());

    for (auto& p : printers) {
        if (!p.is_active) continue;
        pool->submit([this, p]() mutable {
            collect_printer(p);
        });
    }
    pool->wait();
}

void Agent::collect_printer(Printer& p) {
    Log::debug("coleta: {}", p.ip);
    long long ts_now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (p.connection_type == "usb") {
        collect_usb_printer(p, ts_now);
        return;
    }

    // SNMP e a fonte principal; IPP mantem equipamentos sem SNMP monitorados.
    int rtt = snmp->ping(p.ip);
    bool snmp_available = (rtt >= 0);
    bool online = snmp_available;
    if (!online && ipp->check(p.ip)) {
        online = true;
        rtt = 0;
    }

    alerts->check_offline(p.id, online, p.last_seen, ts_now);
    if (!online) {
        Log::warn("coleta: {} nao responde", p.ip);
        PrinterStatus offline;
        offline.printer_id = p.id;
        offline.status = "offline";
        offline.status_message = "SNMP e IPP indisponiveis";
        offline.collected_at = ts_now;
        repo->save_status(offline);
        return;
    }

    repo->update_last_seen(p.id, ts_now, rtt);

    // status
    try {
        auto status = status_module->collect(p, snmp_available);
        repo->save_status(status);
        alerts->check_status(p.id, status);
    } catch (const std::exception& e) {
        Log::warn("coleta status {}: {}", p.ip, e.what());
    }

    // consumiveis
    try {
        auto consumables = consumables_module->collect(p, snmp_available);
        repo->save_consumables(consumables);
        alerts->check_consumables(p.id, consumables);

        // historico de toner
        for (auto& c : consumables) {
            if (c.type == "toner" && c.level_pct >= 0) {
                std::string metric = "toner_" + c.color;
                repo->record_history(p.id, metric, c.level_pct, ts_now);
            }
        }
    } catch (const std::exception& e) {
        Log::warn("coleta consumiveis {}: {}", p.ip, e.what());
    }

    // contadores
    if (snmp_available) try {
        auto counter = counters_module->collect(p);
        repo->save_counter(counter);
        repo->record_history(p.id, "total_pages", counter.total_pages, ts_now);
    } catch (const std::exception& e) {
        Log::warn("coleta contadores {}: {}", p.ip, e.what());
    }

    // rede
    if (snmp_available) try {
        auto net = network_module->collect(p);
        repo->save_network(net);
    } catch (const std::exception& e) {
        Log::warn("coleta rede {}: {}", p.ip, e.what());
    }

    // seguranca — a cada 6 ciclos para nao sobrecarregar
    static std::atomic<int> sec_cycle{0};
    if (snmp_available && ++sec_cycle % 6 == 0) {
        try {
            auto sec = security_module->collect(p);
            repo->save_security(sec);
        } catch (...) {}
    }

    // inventario — verifica mudanca de firmware
    if (snmp_available) try {
        std::string old_fw = p.fw_version;
        auto inv = snmp->collect_inventory(p.ip);
        if (!inv.fw_version.empty() && inv.fw_version != old_fw) {
            alerts->check_firmware_change(p.id, old_fw, inv.fw_version);
        }
        // atualiza modelo/serial caso estejam vazios
        if (p.model.empty() && !inv.model.empty()) {
            p.model = inv.model;
            repo->save_printer(p);
        }
    } catch (...) {}

    try {
        auto jobs = jobs_module->collect(p);
        if (!jobs.empty()) repo->save_jobs(jobs);
    } catch (const std::exception& e) {
        Log::debug("coleta jobs {}: {}", p.ip, e.what());
    }
}

void Agent::collect_usb_printer(Printer& p, long long now) {
    try {
        const auto status = usb->collect_status(p);
        const bool online = status.status != "offline";
        alerts->check_offline(p.id, online, p.last_seen, now);
        repo->save_status(status);
        if (online) repo->update_last_seen(p.id, now, 0);
        alerts->check_status(p.id, status);
    } catch (const std::exception& error) {
        Log::warn("coleta USB {}: {}", p.port_name, error.what());
    }
}
