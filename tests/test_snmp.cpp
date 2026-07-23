#include "core/Config.hpp"
#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

TEST_CASE("SNMPv3 settings are read from configuration", "[snmp][config]") {
    const auto path = std::filesystem::temp_directory_path() / "santa-agent-snmp-test.toml";
    {
        std::ofstream file(path);
        file << "[snmp]\nversion=\"3\"\nsec_name=\"agent\"\nauth_proto=\"SHA\"\n"
                "auth_pass=\"secret-auth\"\npriv_proto=\"AES\"\npriv_pass=\"secret-priv\"\n";
    }
    REQUIRE(Config::get().load(path.string()));
    const auto& snmp = Config::get().data().snmp;
    CHECK(snmp.version == "3");
    CHECK(snmp.sec_name == "agent");
    CHECK(snmp.auth_proto == "SHA");
    CHECK(snmp.priv_proto == "AES");
    std::filesystem::remove(path);
}
