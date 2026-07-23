#pragma once

#include "../notifications/AlertManager.hpp"

class AlertsModule {
public:
    explicit AlertsModule(AlertManager& manager) : manager(manager) {}
    void evaluate(int printer_id, bool online, const PrinterStatus* status,
                  const std::vector<Consumable>& consumables) const;

private:
    AlertManager& manager;
};
