#pragma once

#include "../collectors/SnmpCollector.hpp"

class CountersModule {
public:
    explicit CountersModule(SnmpCollector& snmp) : snmp(snmp) {}
    Counter collect(const Printer& printer) const { return snmp.collect_counters(printer.ip, printer.id); }

private:
    SnmpCollector& snmp;
};
