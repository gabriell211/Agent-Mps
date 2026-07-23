#include "AlertManager.hpp"
#include "../core/Logger.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>

AlertManager::AlertManager(Repository& repo, const AlertThresholds& thresholds)
    : repo(repo), thresholds(thresholds) {}

bool AlertManager::has_open_alert(int printer_id, const std::string& type) {
    auto open = repo.open_alerts(printer_id);
    for (auto& a : open)
        if (a.type == type) return true;
    return false;
}

int AlertManager::raise_alert(int printer_id, const std::string& severity,
                               const std::string& type, const std::string& message) {
    Alert a;
    a.printer_id = printer_id;
    a.severity   = severity;
    a.type       = type;
    a.message    = message;
    std::function<void(const Alert&)> callback;
    int id = -1;
    {
        std::lock_guard<std::mutex> lock(mu);
        if (has_open_alert(printer_id, type)) return -1;
        id = repo.save_alert(a);
        callback = on_raised;
    }
    a.id = id;
    Log::warn("alerta [{}] impressora#{}: {}", severity, printer_id, message);
    if (callback) callback(a);
    return id;
}

void AlertManager::set_on_raised(std::function<void(const Alert&)> callback) {
    std::lock_guard<std::mutex> lock(mu);
    on_raised = std::move(callback);
}

void AlertManager::resolve_by_type(int printer_id, const std::string& type) {
    auto open = repo.open_alerts(printer_id);
    long long ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    for (auto& a : open) {
        if (a.type == type) {
            repo.resolve_alert(a.id, ts);
            Log::info("alerta resolvido [{}] impressora#{}", type, printer_id);
        }
    }
}

void AlertManager::check_consumables(int printer_id, const std::vector<Consumable>& consumables) {
    for (auto& c : consumables) {
        if (c.level_pct < 0) continue;

        std::string type_key = "consumable_" + c.type + "_" + c.color;

        if (c.level_pct <= thresholds.toner_critical) {
            raise_alert(printer_id, "critical", type_key,
                c.name + " critico: " + std::to_string(c.level_pct) + "%");
        }
        else if (c.level_pct <= thresholds.toner_warning) {
            raise_alert(printer_id, "warning", type_key,
                c.name + " baixo: " + std::to_string(c.level_pct) + "%");
        }
        else {
            resolve_by_type(printer_id, type_key);
        }

        if (c.state == "empty") {
            raise_alert(printer_id, "critical", type_key + "_empty",
                c.name + " vazio");
        }
        if (c.state == "missing") {
            raise_alert(printer_id, "critical", type_key + "_missing",
                c.name + " ausente/nao instalado");
        }
    }
}

void AlertManager::check_offline(int printer_id, bool is_online, long long last_seen, long long now) {
    if (!is_online) {
        if (now == 0) {
            now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }
        if (last_seen > 0 && now - last_seen < static_cast<long long>(thresholds.offline_minutes) * 60) return;
        raise_alert(printer_id, "critical", "offline",
            "impressora nao responde");
    } else {
        resolve_by_type(printer_id, "offline");
    }
}

void AlertManager::check_status(int printer_id, const PrinterStatus& status) {
    std::string message = status.status_message;
    std::string code = status.error_code;
    std::transform(message.begin(), message.end(), message.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(code.begin(), code.end(), code.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (message.find("jam") != std::string::npos ||
        message.find("atolamento") != std::string::npos ||
        code.find("jam") != std::string::npos) {
        raise_alert(printer_id, "critical", "paper_jam",
            "papel preso: " + status.status_message);
    } else {
        resolve_by_type(printer_id, "paper_jam");
    }

    // porta aberta
    if (message.find("open") != std::string::npos ||
        message.find("aberta") != std::string::npos) {
        raise_alert(printer_id, "critical", "door_open",
            "porta aberta: " + status.status_message);
    } else {
        resolve_by_type(printer_id, "door_open");
    }

    if (message.find("media-empty") != std::string::npos ||
        message.find("sem papel") != std::string::npos ||
        message.find("no media") != std::string::npos) {
        raise_alert(printer_id, "critical", "paper_out", "bandeja sem papel");
    } else {
        resolve_by_type(printer_id, "paper_out");
    }

    if (status.status == "error" && !status.error_code.empty()) {
        raise_alert(printer_id, "critical", "device_error",
            "erro: " + status.error_code + " " + status.status_message);
    } else {
        resolve_by_type(printer_id, "device_error");
    }
}

void AlertManager::check_firmware_change(int printer_id,
                                          const std::string& old_fw,
                                          const std::string& new_fw) {
    if (!old_fw.empty() && !new_fw.empty() && old_fw != new_fw) {
        raise_alert(printer_id, "warning", "fw_changed",
            "firmware alterado de " + old_fw + " para " + new_fw);
    }
}
