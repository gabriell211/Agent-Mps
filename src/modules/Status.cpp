#include "Status.hpp"

PrinterStatus StatusModule::collect(const Printer& printer, bool snmp_available) const {
    if (snmp_available) {
        try { return snmp.collect_status(printer.ip, printer.id); }
        catch (...) { /* try IPP below */ }
    }
    return ipp.collect_status(printer.ip, printer.id);
}
