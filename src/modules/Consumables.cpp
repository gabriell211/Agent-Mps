#include "Consumables.hpp"

std::vector<Consumable> ConsumablesModule::collect(const Printer& printer, bool snmp_available) const {
    if (snmp_available) {
        try {
            auto items = snmp.collect_consumables(printer.ip, printer.id);
            if (!items.empty()) return items;
        } catch (...) { /* IPP fallback */ }
    }
    return ipp.collect_consumables(printer.ip, printer.id);
}
