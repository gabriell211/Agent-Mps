#pragma once

#include "HttpCollector.hpp"
#include "IppCollector.hpp"
#include "SnmpCollector.hpp"

struct CollectorCapabilities {
    bool snmp = false;
    bool ipp = false;
    bool http = false;
};

class CollectorFactory {
public:
    CollectorFactory(SnmpCollector& snmp, IppCollector& ipp, HttpCollector& http)
        : snmp(snmp), ipp(ipp), http(http) {}

    CollectorCapabilities probe(const std::string& ip) const;

private:
    SnmpCollector& snmp;
    IppCollector& ipp;
    HttpCollector& http;
};
