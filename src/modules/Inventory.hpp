#pragma once

#include "../collectors/HttpCollector.hpp"
#include "../collectors/SnmpCollector.hpp"
#include "../discovery/Discovery.hpp"

class InventoryModule {
public:
    InventoryModule(SnmpCollector& snmp, HttpCollector& http) : snmp(snmp), http(http) {}
    Printer collect(const PrinterEndpoint& endpoint) const;

private:
    SnmpCollector& snmp;
    HttpCollector& http;
};
