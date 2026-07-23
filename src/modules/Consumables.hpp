#pragma once

#include "../collectors/IppCollector.hpp"
#include "../collectors/SnmpCollector.hpp"

class ConsumablesModule {
public:
    ConsumablesModule(SnmpCollector& snmp, IppCollector& ipp) : snmp(snmp), ipp(ipp) {}
    std::vector<Consumable> collect(const Printer& printer, bool snmp_available) const;

private:
    SnmpCollector& snmp;
    IppCollector& ipp;
};
