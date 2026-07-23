#include "discovery/Discovery.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Printer endpoint represents network protocol availability", "[discovery]") {
    PrinterEndpoint endpoint;
    endpoint.ip = "192.168.1.45";
    endpoint.has_snmp = true;
    endpoint.has_ipp = true;

    CHECK(endpoint.ip == "192.168.1.45");
    CHECK(endpoint.has_snmp);
    CHECK(endpoint.has_ipp);
    CHECK_FALSE(endpoint.has_http);
}
