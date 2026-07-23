#include "Alerts.hpp"

void AlertsModule::evaluate(int printer_id, bool online, const PrinterStatus* status,
                            const std::vector<Consumable>& consumables) const {
    manager.check_offline(printer_id, online);
    if (status) manager.check_status(printer_id, *status);
    if (!consumables.empty()) manager.check_consumables(printer_id, consumables);
}
