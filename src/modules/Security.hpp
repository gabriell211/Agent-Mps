#pragma once

#include "../collectors/SnmpCollector.hpp"

class SecurityModule {
public:
    explicit SecurityModule(SnmpCollector& snmp) : snmp(snmp) {}
    SecurityInfo collect(const Printer& printer) const { return snmp.collect_security(printer.ip, printer.id); }

private:
    SnmpCollector& snmp;
};
