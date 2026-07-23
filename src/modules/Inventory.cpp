#include "Inventory.hpp"
#include "../core/Logger.hpp"

Printer InventoryModule::collect(const PrinterEndpoint& endpoint) const {
    Printer printer;
    printer.ip = endpoint.ip;
    printer.hostname = endpoint.hostname;
    printer.mac = endpoint.mac;
    printer.connection_type = "network";

    if (endpoint.has_snmp) {
        try {
            printer = snmp.collect_inventory(endpoint.ip);
            if (!endpoint.hostname.empty()) printer.hostname = endpoint.hostname;
            if (!endpoint.mac.empty()) printer.mac = endpoint.mac;
        } catch (const std::exception& error) {
            Log::warn("inventario SNMP de {} falhou: {}", endpoint.ip, error.what());
        }
    }
    if (printer.model.empty() && endpoint.has_http) {
        try {
            Printer from_http = http.collect_inventory_ews(endpoint.ip, printer.manufacturer);
            if (!from_http.model.empty()) printer.model = from_http.model;
            if (!from_http.serial.empty()) printer.serial = from_http.serial;
            if (!from_http.fw_version.empty()) printer.fw_version = from_http.fw_version;
        } catch (const std::exception& error) {
            Log::debug("inventario HTTP de {} falhou: {}", endpoint.ip, error.what());
        }
    }
    return printer;
}
