#include "storage/Database.hpp"
#include "storage/Migrations.hpp"
#include "storage/Repository.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Repository persists transport, status and counters", "[storage]") {
    Database database;
    database.open(":memory:");
    Migrations::run(database);
    Repository repository(database);

    Printer printer;
    printer.ip = "usb://USB001/Fila";
    printer.connection_type = "usb";
    printer.port_name = "USB001";
    printer.model = "Driver de teste";
    const int id = repository.save_printer(printer);
    REQUIRE(id > 0);

    const auto loaded = repository.find_printer(id);
    REQUIRE(loaded);
    CHECK(loaded->connection_type == "usb");
    CHECK(loaded->port_name == "USB001");

    PrinterStatus status;
    status.printer_id = id;
    status.status = "idle";
    status.collected_at = 100;
    repository.save_status(status);
    REQUIRE(repository.latest_status(id));
    CHECK(repository.latest_status(id)->status == "idle");

    Counter counter;
    counter.printer_id = id;
    counter.total_pages = 42;
    counter.usb_pages = 42;
    counter.collected_at = 101;
    repository.save_counter(counter);
    REQUIRE(repository.latest_counter(id));
    CHECK(repository.latest_counter(id)->usb_pages == 42);

    repository.save_event(id, "syslog", "paper-low", 102);
    REQUIRE(repository.latest_events(id).size() == 1);
    CHECK(repository.latest_events(id).front().message == "paper-low");
}
