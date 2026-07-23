#include "Migrations.hpp"
#include "../core/Logger.hpp"

int Migrations::current_version(Database& db) {
    int ver = 0;
    db.query("PRAGMA user_version", [&](sqlite3_stmt* s) {
        ver = sqlite3_column_int(s, 0);
        return false;
    });
    return ver;
}

void Migrations::set_version(Database& db, int v) {
    db.exec("PRAGMA user_version = " + std::to_string(v));
}

void Migrations::run(Database& db) {
    int ver = current_version(db);
    Log::info("migrations: versao atual do banco = {}", ver);

    if (ver < 1) { v1_create_tables(db); set_version(db, 1); Log::info("migrations: v1 aplicada"); }
    if (ver < 2) { v2_add_security(db);  set_version(db, 2); Log::info("migrations: v2 aplicada"); }
    if (ver < 3) { v3_add_jobs(db);      set_version(db, 3); Log::info("migrations: v3 aplicada"); }
    if (ver < 4) { v4_add_transport_fields(db); set_version(db, 4); Log::info("migrations: v4 aplicada"); }
    if (ver < 5) { v5_add_events_and_logs(db); set_version(db, 5); Log::info("migrations: v5 aplicada"); }
}

void Migrations::v1_create_tables(Database& db) {
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printers (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            ip              TEXT    UNIQUE NOT NULL,
            hostname        TEXT,
            mac             TEXT,
            manufacturer    TEXT,
            model           TEXT,
            friendly_name   TEXT,
            serial          TEXT,
            asset_tag       TEXT,
            uuid            TEXT,
            memory_mb       INTEGER DEFAULT 0,
            cpu             TEXT,
            flash           TEXT,
            has_duplex      INTEGER DEFAULT 0,
            has_scanner     INTEGER DEFAULT 0,
            has_fax         INTEGER DEFAULT 0,
            has_stapler     INTEGER DEFAULT 0,
            tray_count      INTEGER DEFAULT 0,
            fw_version      TEXT,
            fw_build        TEXT,
            fw_date         TEXT,
            fw_engine       TEXT,
            bootloader      TEXT,
            is_active       INTEGER DEFAULT 1,
            discovered_at   INTEGER,
            last_seen       INTEGER,
            response_ms     INTEGER DEFAULT 0
        )
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printer_status (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            printer_id      INTEGER NOT NULL REFERENCES printers(id),
            status          TEXT,
            status_message  TEXT,
            error_code      TEXT,
            uptime_s        INTEGER DEFAULT 0,
            collected_at    INTEGER NOT NULL
        )
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printer_consumables (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            printer_id      INTEGER NOT NULL REFERENCES printers(id),
            name            TEXT,
            type            TEXT,
            color           TEXT,
            capacity_pages  INTEGER DEFAULT 0,
            remaining_pages INTEGER DEFAULT 0,
            level_pct       INTEGER DEFAULT -1,
            state           TEXT,
            collected_at    INTEGER NOT NULL
        )
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printer_counters (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            printer_id      INTEGER NOT NULL REFERENCES printers(id),
            total_pages     INTEGER DEFAULT 0,
            mono_pages      INTEGER DEFAULT 0,
            color_pages     INTEGER DEFAULT 0,
            simplex_pages   INTEGER DEFAULT 0,
            duplex_pages    INTEGER DEFAULT 0,
            copy_pages      INTEGER DEFAULT 0,
            print_pages     INTEGER DEFAULT 0,
            scan_pages      INTEGER DEFAULT 0,
            fax_pages       INTEGER DEFAULT 0,
            adf_pages       INTEGER DEFAULT 0,
            usb_pages       INTEGER DEFAULT 0,
            net_pages       INTEGER DEFAULT 0,
            secure_pages    INTEGER DEFAULT 0,
            collected_at    INTEGER NOT NULL
        )
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printer_network (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            printer_id      INTEGER NOT NULL REFERENCES printers(id),
            ipv4            TEXT,
            ipv6            TEXT,
            gateway         TEXT,
            dns             TEXT,
            subnet          TEXT,
            dhcp            INTEGER DEFAULT 0,
            speed_mbps      INTEGER DEFAULT 0,
            uptime_s        INTEGER DEFAULT 0,
            iface_type      TEXT,
            collected_at    INTEGER NOT NULL
        )
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printer_alerts (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            printer_id      INTEGER NOT NULL REFERENCES printers(id),
            severity        TEXT,
            type            TEXT,
            message         TEXT,
            acknowledged    INTEGER DEFAULT 0,
            created_at      INTEGER NOT NULL,
            resolved_at     INTEGER DEFAULT 0
        )
    )");

    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printer_history (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            printer_id      INTEGER NOT NULL REFERENCES printers(id),
            metric          TEXT NOT NULL,
            value           REAL,
            recorded_at     INTEGER NOT NULL
        )
    )");

    // indices basicos
    db.exec("CREATE INDEX IF NOT EXISTS idx_status_printer   ON printer_status(printer_id, collected_at)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_cons_printer     ON printer_consumables(printer_id, collected_at)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_counter_printer  ON printer_counters(printer_id, collected_at)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_alert_printer    ON printer_alerts(printer_id, resolved_at)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_hist_printer_met ON printer_history(printer_id, metric, recorded_at)");
}

void Migrations::v2_add_security(Database& db) {
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printer_security (
            id               INTEGER PRIMARY KEY AUTOINCREMENT,
            printer_id       INTEGER NOT NULL REFERENCES printers(id),
            snmp_version     TEXT,
            snmp_community   TEXT,
            http_enabled     INTEGER DEFAULT 0,
            https_enabled    INTEGER DEFAULT 0,
            telnet_enabled   INTEGER DEFAULT 0,
            ftp_enabled      INTEGER DEFAULT 0,
            ssh_enabled      INTEGER DEFAULT 0,
            smb_enabled      INTEGER DEFAULT 0,
            tls_version      TEXT,
            cert_expiry      TEXT,
            default_password INTEGER DEFAULT 0,
            open_ports       TEXT,
            collected_at     INTEGER NOT NULL
        )
    )");
}

void Migrations::v3_add_jobs(Database& db) {
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printer_jobs (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            printer_id  INTEGER NOT NULL REFERENCES printers(id),
            job_id      INTEGER,
            job_name    TEXT,
            user_name   TEXT,
            status      TEXT,
            pages       INTEGER DEFAULT 0,
            created_at  INTEGER NOT NULL
        )
    )");
}

void Migrations::v4_add_transport_fields(Database& db) {
    // ALTER TABLE is deliberately additive so existing installations retain
    // their discovery history.  USB endpoints use a stable usb:// key in the
    // legacy ip column while these fields expose the real transport.
    db.exec("ALTER TABLE printers ADD COLUMN connection_type TEXT NOT NULL DEFAULT 'network'");
    db.exec("ALTER TABLE printers ADD COLUMN port_name TEXT");
    db.exec("CREATE INDEX IF NOT EXISTS idx_printers_transport ON printers(connection_type, port_name)");
}

void Migrations::v5_add_events_and_logs(Database& db) {
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printer_events (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            printer_id  INTEGER REFERENCES printers(id),
            type        TEXT NOT NULL,
            message     TEXT NOT NULL,
            created_at  INTEGER NOT NULL
        )
    )");
    db.exec(R"(
        CREATE TABLE IF NOT EXISTS printer_logs (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            printer_id  INTEGER REFERENCES printers(id),
            source      TEXT NOT NULL,
            message     TEXT NOT NULL,
            created_at  INTEGER NOT NULL
        )
    )");
    db.exec("CREATE INDEX IF NOT EXISTS idx_events_printer ON printer_events(printer_id, created_at)");
    db.exec("CREATE INDEX IF NOT EXISTS idx_logs_printer ON printer_logs(printer_id, created_at)");
}
