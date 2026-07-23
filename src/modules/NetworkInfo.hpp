#pragma once

#include "../collectors/SnmpCollector.hpp"

class NetworkInfoModule {
public:
    explicit NetworkInfoModule(SnmpCollector& snmp) : snmp(snmp) {}
    NetworkConfig collect(const Printer& printer) const { return snmp.collect_network(printer.ip, printer.id); }

private:
    SnmpCollector& snmp;
};
