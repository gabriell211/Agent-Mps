cmake_minimum_required(VERSION 3.20)
project(santa_agent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(httplib CONFIG REQUIRED)
if(WIN32)
    # Net-SNMP is normally installed with Chocolatey/MSYS2 or provided by the
    # deployment toolchain.  Keep it explicit so a missing runtime cannot
    # produce a partially functional agent.
    find_path(NETSNMP_INCLUDE_DIRS net-snmp/net-snmp-config.h REQUIRED)
    find_library(NETSNMP_LIBRARIES NAMES netsnmp REQUIRED)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NETSNMP REQUIRED netsnmp)
endif()

set(SOURCES
    src/main.cpp
    src/core/Agent.cpp
    src/core/Config.cpp
    src/core/Logger.cpp
    src/core/Scheduler.cpp
    src/core/ThreadPool.cpp
    src/storage/Database.cpp
    src/storage/Migrations.cpp
    src/storage/Repository.cpp
    src/collectors/SnmpCollector.cpp
    src/collectors/IppCollector.cpp
    src/collectors/HttpCollector.cpp
    src/collectors/UsbCollector.cpp
    src/collectors/CollectorFactory.cpp
    src/modules/Inventory.cpp
    src/modules/Status.cpp
    src/modules/Consumables.cpp
    src/modules/Counters.cpp
    src/modules/NetworkInfo.cpp
    src/modules/Security.cpp
    src/modules/Jobs.cpp
    src/modules/Alerts.cpp
    src/discovery/Discovery.cpp
    src/discovery/IpScanner.cpp
    src/discovery/ArpScanner.cpp
    src/discovery/SnmpDiscovery.cpp
    src/discovery/MdnsDiscovery.cpp
    src/api/RestServer.cpp
    src/api/Router.cpp
    src/api/Serializer.cpp
    src/notifications/AlertManager.cpp
    src/notifications/WebhookNotifier.cpp
    src/notifications/EmailNotifier.cpp
    src/notifications/SyslogListener.cpp
)

add_executable(santa_agent ${SOURCES})

target_include_directories(santa_agent PRIVATE
    src/
    ${NETSNMP_INCLUDE_DIRS}
)

target_link_libraries(santa_agent PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    SQLite::SQLite3
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
    httplib::httplib
    ${NETSNMP_LIBRARIES}
)

target_compile_options(santa_agent PRIVATE ${NETSNMP_CFLAGS_OTHER})

if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(santa_agent PRIVATE Threads::Threads)
endif()

if(WIN32)
    target_link_libraries(santa_agent PRIVATE ws2_32 winspool iphlpapi)
endif()

include(GNUInstallDirs)
install(TARGETS santa_agent RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES LICENSE NOTICE README.md config.example.toml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/santa-agent)
install(FILES docs/HOMOLOGATION.md
        DESTINATION ${CMAKE_INSTALL_DATADIR}/santa-agent/docs)

# testes
option(BUILD_TESTS "build testes" ON)
if(BUILD_TESTS)
    find_package(Catch2 3 CONFIG REQUIRED)
    add_executable(run_tests
        tests/test_snmp.cpp
        tests/test_discovery.cpp
        tests/test_storage.cpp
        src/core/Config.cpp
        src/core/Logger.cpp
        src/storage/Database.cpp
        src/storage/Migrations.cpp
        src/storage/Repository.cpp
    )
    target_include_directories(run_tests PRIVATE src/)
    target_link_libraries(run_tests PRIVATE
        Catch2::Catch2WithMain
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        SQLite::SQLite3
        OpenSSL::SSL
        OpenSSL::Crypto
    )
    include(CTest)
    include(Catch)
    catch_discover_tests(run_tests)
endif()
