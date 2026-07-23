#include "CollectorFactory.hpp"

CollectorCapabilities CollectorFactory::probe(const std::string& ip) const {
    CollectorCapabilities result;
    result.snmp = snmp.ping(ip) >= 0;
    if (!result.snmp) result.ipp = ipp.check(ip);
    result.http = !http.fetch("http://" + ip + "/", 3).empty();
    return result;
}
