#pragma once

#include "../collectors/IppCollector.hpp"
#include "../collectors/SnmpCollector.hpp"

class StatusModule {
public:
    StatusModule(SnmpCollector& snmp, IppCollector& ipp) : snmp(snmp), ipp(ipp) {}
    PrinterStatus collect(const Printer& printer, bool snmp_available) const;

private:
    SnmpCollector& snmp;
    IppCollector& ipp;
};
