#include "Repository.hpp"
#include <sqlite3.h>
#include <sstream>
#include <chrono>
#include <mutex>
#include <algorithm>

static long long now_epoch() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

static std::string col_text(sqlite3_stmt* s, int i) {
    auto p = sqlite3_column_text(s, i);
    return p ? reinterpret_cast<const char*>(p) : "";
}

int Repository::save_printer(const Printer& p) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto existing = find_printer_by_ip(p.ip);
    if (existing) {
        auto stmt = db.prepare(R"(
            UPDATE printers SET
                hostname=?,mac=?,manufacturer=?,model=?,friendly_name=?,serial=?,asset_tag=?,uuid=?,
                memory_mb=?,cpu=?,flash=?,has_duplex=?,has_scanner=?,has_fax=?,has_stapler=?,
                tray_count=?,fw_version=?,fw_build=?,fw_date=?,fw_engine=?,bootloader=?,
                last_seen=?,response_ms=?,is_active=1,connection_type=?,port_name=?
            WHERE ip=?
        )");
        sqlite3_bind_text(stmt, 1,  p.hostname.c_str(),      -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2,  p.mac.c_str(),           -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3,  p.manufacturer.c_str(),  -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4,  p.model.c_str(),         -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5,  p.friendly_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6,  p.serial.c_str(),        -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7,  p.asset_tag.c_str(),     -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 8,  p.uuid.c_str(),          -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt,  9,  p.memory_mb);
        sqlite3_bind_text(stmt, 10, p.cpu.c_str(),           -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 11, p.flash.c_str(),         -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt,  12, p.has_duplex  ? 1 : 0);
        sqlite3_bind_int(stmt,  13, p.has_scanner ? 1 : 0);
        sqlite3_bind_int(stmt,  14, p.has_fax     ? 1 : 0);
        sqlite3_bind_int(stmt,  15, p.has_stapler ? 1 : 0);
        sqlite3_bind_int(stmt,  16, p.tray_count);
        sqlite3_bind_text(stmt, 17, p.fw_version.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 18, p.fw_build.c_str(),   -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 19, p.fw_date.c_str(),    -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 20, p.fw_engine.c_str(),  -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 21, p.bootloader.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt,22, now_epoch());
        sqlite3_bind_int(stmt,  23, p.response_ms);
        sqlite3_bind_text(stmt, 24, p.connection_type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 25, p.port_name.c_str(),       -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 26, p.ip.c_str(),              -1, SQLITE_TRANSIENT);
        db.step(stmt);
        db.finalize(stmt);
        return existing->id;
    }

    auto stmt = db.prepare(R"(
        INSERT INTO printers(ip,hostname,mac,manufacturer,model,friendly_name,serial,asset_tag,uuid,
            memory_mb,cpu,flash,has_duplex,has_scanner,has_fax,has_stapler,tray_count,
            fw_version,fw_build,fw_date,fw_engine,bootloader,discovered_at,last_seen,response_ms,connection_type,port_name)
        VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
    )");
    sqlite3_bind_text(stmt, 1,  p.ip.c_str(),           -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2,  p.hostname.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3,  p.mac.c_str(),          -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4,  p.manufacturer.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5,  p.model.c_str(),        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6,  p.friendly_name.c_str(),-1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7,  p.serial.c_str(),       -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8,  p.asset_tag.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9,  p.uuid.c_str(),         -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  10, p.memory_mb);
    sqlite3_bind_text(stmt, 11, p.cpu.c_str(),          -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, p.flash.c_str(),        -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  13, p.has_duplex  ? 1 : 0);
    sqlite3_bind_int(stmt,  14, p.has_scanner ? 1 : 0);
    sqlite3_bind_int(stmt,  15, p.has_fax     ? 1 : 0);
    sqlite3_bind_int(stmt,  16, p.has_stapler ? 1 : 0);
    sqlite3_bind_int(stmt,  17, p.tray_count);
    sqlite3_bind_text(stmt, 18, p.fw_version.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 19, p.fw_build.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 20, p.fw_date.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 21, p.fw_engine.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 22, p.bootloader.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt,23, now_epoch());
    sqlite3_bind_int64(stmt,24, now_epoch());
    sqlite3_bind_int(stmt,  25, p.response_ms);
    sqlite3_bind_text(stmt, 26, p.connection_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 27, p.port_name.c_str(),       -1, SQLITE_TRANSIENT);
    db.step(stmt);
    db.finalize(stmt);
    return static_cast<int>(db.last_insert_id());
}

void Repository::update_last_seen(int id, long long ts, int rtt_ms) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare("UPDATE printers SET last_seen=?, response_ms=?, is_active=1 WHERE id=?");
    sqlite3_bind_int64(stmt, 1, ts);
    sqlite3_bind_int(stmt,  2, rtt_ms);
    sqlite3_bind_int(stmt,  3, id);
    db.step(stmt);
    db.finalize(stmt);
}

std::optional<Printer> Repository::find_printer_by_ip(const std::string& ip) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    std::optional<Printer> result;
    auto stmt = db.prepare("SELECT id,ip,hostname,mac,manufacturer,model,serial,fw_version,response_ms,is_active,connection_type,port_name FROM printers WHERE ip=? LIMIT 1");
    sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_TRANSIENT);
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Printer p;
        p.id           = sqlite3_column_int(stmt, 0);
        p.ip           = col_text(stmt, 1);
        p.hostname     = col_text(stmt, 2);
        p.mac          = col_text(stmt, 3);
        p.manufacturer = col_text(stmt, 4);
        p.model        = col_text(stmt, 5);
        p.serial       = col_text(stmt, 6);
        p.fw_version   = col_text(stmt, 7);
        p.response_ms  = sqlite3_column_int(stmt, 8);
        p.is_active    = sqlite3_column_int(stmt, 9) != 0;
        p.connection_type = col_text(stmt, 10);
        p.port_name = col_text(stmt, 11);
        result = p;
    }
    db.finalize(stmt);
    return result;
}

std::optional<Printer> Repository::find_printer(int id) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare("SELECT id,ip,hostname,mac,manufacturer,model,serial,fw_version,response_ms,is_active,last_seen,connection_type,port_name FROM printers WHERE id=? LIMIT 1");
    sqlite3_bind_int(stmt, 1, id);
    std::optional<Printer> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Printer p;
        p.id = sqlite3_column_int(stmt, 0);
        p.ip = col_text(stmt, 1);
        p.hostname = col_text(stmt, 2);
        p.mac = col_text(stmt, 3);
        p.manufacturer = col_text(stmt, 4);
        p.model = col_text(stmt, 5);
        p.serial = col_text(stmt, 6);
        p.fw_version = col_text(stmt, 7);
        p.response_ms = sqlite3_column_int(stmt, 8);
        p.is_active = sqlite3_column_int(stmt, 9) != 0;
        p.last_seen = sqlite3_column_int64(stmt, 10);
        p.connection_type = col_text(stmt, 11);
        p.port_name = col_text(stmt, 12);
        result = p;
    }
    db.finalize(stmt);
    return result;
}

std::vector<Printer> Repository::all_printers() {
    std::lock_guard<std::recursive_mutex> lock(mu);
    std::vector<Printer> out;
    auto stmt = db.prepare("SELECT id,ip,hostname,mac,manufacturer,model,serial,fw_version,response_ms,is_active,last_seen,connection_type,port_name FROM printers ORDER BY ip");
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Printer p;
        p.id           = sqlite3_column_int(stmt, 0);
        p.ip           = col_text(stmt, 1);
        p.hostname     = col_text(stmt, 2);
        p.mac          = col_text(stmt, 3);
        p.manufacturer = col_text(stmt, 4);
        p.model        = col_text(stmt, 5);
        p.serial       = col_text(stmt, 6);
        p.fw_version   = col_text(stmt, 7);
        p.response_ms  = sqlite3_column_int(stmt, 8);
        p.is_active    = sqlite3_column_int(stmt, 9) != 0;
        p.last_seen    = sqlite3_column_int64(stmt, 10);
        p.connection_type = col_text(stmt, 11);
        p.port_name = col_text(stmt, 12);
        out.push_back(p);
    }
    db.finalize(stmt);
    return out;
}

void Repository::mark_inactive(int id) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare("UPDATE printers SET is_active=0 WHERE id=?");
    sqlite3_bind_int(stmt, 1, id);
    db.step(stmt);
    db.finalize(stmt);
}

void Repository::save_status(const PrinterStatus& s) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare(
        "INSERT INTO printer_status(printer_id,status,status_message,error_code,uptime_s,collected_at) "
        "VALUES(?,?,?,?,?,?)"
    );
    sqlite3_bind_int(stmt,  1, s.printer_id);
    sqlite3_bind_text(stmt, 2, s.status.c_str(),         -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, s.status_message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, s.error_code.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt,5, s.uptime_s);
    sqlite3_bind_int64(stmt,6, s.collected_at ? s.collected_at : now_epoch());
    db.step(stmt);
    db.finalize(stmt);
}

std::optional<PrinterStatus> Repository::latest_status(int printer_id) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare("SELECT printer_id,status,status_message,error_code,uptime_s,collected_at FROM printer_status WHERE printer_id=? ORDER BY collected_at DESC,id DESC LIMIT 1");
    sqlite3_bind_int(stmt, 1, printer_id);
    std::optional<PrinterStatus> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        PrinterStatus s;
        s.printer_id = sqlite3_column_int(stmt, 0);
        s.status = col_text(stmt, 1);
        s.status_message = col_text(stmt, 2);
        s.error_code = col_text(stmt, 3);
        s.uptime_s = sqlite3_column_int64(stmt, 4);
        s.collected_at = sqlite3_column_int64(stmt, 5);
        result = s;
    }
    db.finalize(stmt);
    return result;
}

void Repository::save_consumables(const std::vector<Consumable>& items) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    long long ts = now_epoch();
    db.begin();
    try {
        for (auto& c : items) {
            auto stmt = db.prepare(
                "INSERT INTO printer_consumables(printer_id,name,type,color,capacity_pages,remaining_pages,level_pct,state,collected_at) "
                "VALUES(?,?,?,?,?,?,?,?,?)"
            );
            sqlite3_bind_int(stmt,  1, c.printer_id);
            sqlite3_bind_text(stmt, 2, c.name.c_str(),  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, c.type.c_str(),  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, c.color.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt,  5, c.capacity_pages);
            sqlite3_bind_int(stmt,  6, c.remaining_pages);
            sqlite3_bind_int(stmt,  7, c.level_pct);
            sqlite3_bind_text(stmt, 8, c.state.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt,9, ts);
            db.step(stmt);
            db.finalize(stmt);
        }
        db.commit();
    } catch (...) {
        db.rollback();
        throw;
    }
}

std::vector<Consumable> Repository::latest_consumables(int printer_id) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    std::vector<Consumable> out;
    auto stmt = db.prepare(R"(
        SELECT name,type,color,capacity_pages,remaining_pages,level_pct,state
        FROM printer_consumables
        WHERE printer_id=?
          AND collected_at = (SELECT MAX(collected_at) FROM printer_consumables WHERE printer_id=?)
    )");
    sqlite3_bind_int(stmt, 1, printer_id);
    sqlite3_bind_int(stmt, 2, printer_id);
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Consumable c;
        c.printer_id      = printer_id;
        c.name            = col_text(stmt, 0);
        c.type            = col_text(stmt, 1);
        c.color           = col_text(stmt, 2);
        c.capacity_pages  = sqlite3_column_int(stmt, 3);
        c.remaining_pages = sqlite3_column_int(stmt, 4);
        c.level_pct       = sqlite3_column_int(stmt, 5);
        c.state           = col_text(stmt, 6);
        out.push_back(c);
    }
    db.finalize(stmt);
    return out;
}

void Repository::save_counter(const Counter& c) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare(
        "INSERT INTO printer_counters(printer_id,total_pages,mono_pages,color_pages,simplex_pages,"
        "duplex_pages,copy_pages,print_pages,scan_pages,fax_pages,adf_pages,usb_pages,net_pages,secure_pages,collected_at) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
    );
    sqlite3_bind_int(stmt,  1, c.printer_id);
    sqlite3_bind_int64(stmt,2, c.total_pages);
    sqlite3_bind_int64(stmt,3, c.mono_pages);
    sqlite3_bind_int64(stmt,4, c.color_pages);
    sqlite3_bind_int64(stmt,5, c.simplex_pages);
    sqlite3_bind_int64(stmt,6, c.duplex_pages);
    sqlite3_bind_int64(stmt,7, c.copy_pages);
    sqlite3_bind_int64(stmt,8, c.print_pages);
    sqlite3_bind_int64(stmt,9, c.scan_pages);
    sqlite3_bind_int64(stmt,10,c.fax_pages);
    sqlite3_bind_int64(stmt,11,c.adf_pages);
    sqlite3_bind_int64(stmt,12,c.usb_pages);
    sqlite3_bind_int64(stmt,13,c.net_pages);
    sqlite3_bind_int64(stmt,14,c.secure_pages);
    sqlite3_bind_int64(stmt,15,c.collected_at ? c.collected_at : now_epoch());
    db.step(stmt);
    db.finalize(stmt);
}

std::optional<Counter> Repository::latest_counter(int printer_id) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare("SELECT printer_id,total_pages,mono_pages,color_pages,simplex_pages,duplex_pages,copy_pages,print_pages,scan_pages,fax_pages,adf_pages,usb_pages,net_pages,secure_pages,collected_at FROM printer_counters WHERE printer_id=? ORDER BY collected_at DESC,id DESC LIMIT 1");
    sqlite3_bind_int(stmt, 1, printer_id);
    std::optional<Counter> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Counter c;
        c.printer_id = sqlite3_column_int(stmt, 0);
        c.total_pages = sqlite3_column_int64(stmt, 1);
        c.mono_pages = sqlite3_column_int64(stmt, 2);
        c.color_pages = sqlite3_column_int64(stmt, 3);
        c.simplex_pages = sqlite3_column_int64(stmt, 4);
        c.duplex_pages = sqlite3_column_int64(stmt, 5);
        c.copy_pages = sqlite3_column_int64(stmt, 6);
        c.print_pages = sqlite3_column_int64(stmt, 7);
        c.scan_pages = sqlite3_column_int64(stmt, 8);
        c.fax_pages = sqlite3_column_int64(stmt, 9);
        c.adf_pages = sqlite3_column_int64(stmt, 10);
        c.usb_pages = sqlite3_column_int64(stmt, 11);
        c.net_pages = sqlite3_column_int64(stmt, 12);
        c.secure_pages = sqlite3_column_int64(stmt, 13);
        c.collected_at = sqlite3_column_int64(stmt, 14);
        result = c;
    }
    db.finalize(stmt);
    return result;
}

void Repository::save_network(const NetworkConfig& n) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare(
        "INSERT INTO printer_network(printer_id,ipv4,ipv6,gateway,dns,subnet,dhcp,speed_mbps,uptime_s,iface_type,collected_at) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?)"
    );
    sqlite3_bind_int(stmt,  1, n.printer_id);
    sqlite3_bind_text(stmt, 2, n.ipv4.c_str(),       -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, n.ipv6.c_str(),       -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, n.gateway.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, n.dns.c_str(),        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, n.subnet.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  7, n.dhcp ? 1 : 0);
    sqlite3_bind_int(stmt,  8, n.speed_mbps);
    sqlite3_bind_int64(stmt,9, n.uptime_s);
    sqlite3_bind_text(stmt, 10,n.iface_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt,11,n.collected_at ? n.collected_at : now_epoch());
    db.step(stmt);
    db.finalize(stmt);
}

std::optional<NetworkConfig> Repository::latest_network(int printer_id) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare("SELECT printer_id,ipv4,ipv6,gateway,dns,subnet,dhcp,speed_mbps,uptime_s,iface_type,collected_at FROM printer_network WHERE printer_id=? ORDER BY collected_at DESC,id DESC LIMIT 1");
    sqlite3_bind_int(stmt, 1, printer_id);
    std::optional<NetworkConfig> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        NetworkConfig n;
        n.printer_id = sqlite3_column_int(stmt, 0);
        n.ipv4 = col_text(stmt, 1);
        n.ipv6 = col_text(stmt, 2);
        n.gateway = col_text(stmt, 3);
        n.dns = col_text(stmt, 4);
        n.subnet = col_text(stmt, 5);
        n.dhcp = sqlite3_column_int(stmt, 6) != 0;
        n.speed_mbps = sqlite3_column_int(stmt, 7);
        n.uptime_s = sqlite3_column_int64(stmt, 8);
        n.iface_type = col_text(stmt, 9);
        n.collected_at = sqlite3_column_int64(stmt, 10);
        result = n;
    }
    db.finalize(stmt);
    return result;
}

void Repository::save_security(const SecurityInfo& s) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    std::string ports_str;
    for (size_t i = 0; i < s.open_ports.size(); i++) {
        if (i) ports_str += ',';
        ports_str += std::to_string(s.open_ports[i]);
    }
    auto stmt = db.prepare(
        "INSERT INTO printer_security(printer_id,snmp_version,snmp_community,http_enabled,https_enabled,"
        "telnet_enabled,ftp_enabled,ssh_enabled,smb_enabled,tls_version,cert_expiry,default_password,open_ports,collected_at) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
    );
    sqlite3_bind_int(stmt,  1, s.printer_id);
    sqlite3_bind_text(stmt, 2, s.snmp_version.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, s.snmp_community.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  4, s.http_enabled     ? 1 : 0);
    sqlite3_bind_int(stmt,  5, s.https_enabled    ? 1 : 0);
    sqlite3_bind_int(stmt,  6, s.telnet_enabled   ? 1 : 0);
    sqlite3_bind_int(stmt,  7, s.ftp_enabled      ? 1 : 0);
    sqlite3_bind_int(stmt,  8, s.ssh_enabled      ? 1 : 0);
    sqlite3_bind_int(stmt,  9, s.smb_enabled      ? 1 : 0);
    sqlite3_bind_text(stmt, 10,s.tls_version.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11,s.cert_expiry.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  12,s.default_password ? 1 : 0);
    sqlite3_bind_text(stmt, 13,ports_str.c_str(),        -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt,14,s.collected_at ? s.collected_at : now_epoch());
    db.step(stmt);
    db.finalize(stmt);
}

std::optional<SecurityInfo> Repository::latest_security(int printer_id) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare("SELECT printer_id,snmp_version,snmp_community,http_enabled,https_enabled,telnet_enabled,ftp_enabled,ssh_enabled,smb_enabled,tls_version,cert_expiry,default_password,open_ports,collected_at FROM printer_security WHERE printer_id=? ORDER BY collected_at DESC,id DESC LIMIT 1");
    sqlite3_bind_int(stmt, 1, printer_id);
    std::optional<SecurityInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        SecurityInfo s;
        s.printer_id = sqlite3_column_int(stmt, 0);
        s.snmp_version = col_text(stmt, 1);
        s.snmp_community = col_text(stmt, 2);
        s.http_enabled = sqlite3_column_int(stmt, 3) != 0;
        s.https_enabled = sqlite3_column_int(stmt, 4) != 0;
        s.telnet_enabled = sqlite3_column_int(stmt, 5) != 0;
        s.ftp_enabled = sqlite3_column_int(stmt, 6) != 0;
        s.ssh_enabled = sqlite3_column_int(stmt, 7) != 0;
        s.smb_enabled = sqlite3_column_int(stmt, 8) != 0;
        s.tls_version = col_text(stmt, 9);
        s.cert_expiry = col_text(stmt, 10);
        s.default_password = sqlite3_column_int(stmt, 11) != 0;
        std::stringstream ports(col_text(stmt, 12));
        std::string item;
        while (std::getline(ports, item, ',')) if (!item.empty()) s.open_ports.push_back(std::stoi(item));
        s.collected_at = sqlite3_column_int64(stmt, 13);
        result = s;
    }
    db.finalize(stmt);
    return result;
}

int Repository::save_alert(const Alert& a) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare(
        "INSERT INTO printer_alerts(printer_id,severity,type,message,created_at) VALUES(?,?,?,?,?)"
    );
    sqlite3_bind_int(stmt,  1, a.printer_id);
    sqlite3_bind_text(stmt, 2, a.severity.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, a.type.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, a.message.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt,5, a.created_at ? a.created_at : now_epoch());
    db.step(stmt);
    db.finalize(stmt);
    return static_cast<int>(db.last_insert_id());
}

void Repository::resolve_alert(int id, long long ts) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare("UPDATE printer_alerts SET resolved_at=? WHERE id=?");
    sqlite3_bind_int64(stmt, 1, ts ? ts : now_epoch());
    sqlite3_bind_int(stmt,   2, id);
    db.step(stmt);
    db.finalize(stmt);
}

std::vector<Alert> Repository::open_alerts(int printer_id) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    std::vector<Alert> out;
    auto stmt = db.prepare(
        "SELECT id,printer_id,severity,type,message,acknowledged,created_at FROM printer_alerts "
        "WHERE printer_id=? AND resolved_at=0 ORDER BY created_at DESC"
    );
    sqlite3_bind_int(stmt, 1, printer_id);
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Alert a;
        a.id           = sqlite3_column_int(stmt, 0);
        a.printer_id   = sqlite3_column_int(stmt, 1);
        a.severity     = col_text(stmt, 2);
        a.type         = col_text(stmt, 3);
        a.message      = col_text(stmt, 4);
        a.acknowledged = sqlite3_column_int(stmt, 5) != 0;
        a.created_at   = sqlite3_column_int64(stmt, 6);
        out.push_back(a);
    }
    db.finalize(stmt);
    return out;
}

std::vector<Alert> Repository::all_open_alerts() {
    std::lock_guard<std::recursive_mutex> lock(mu);
    std::vector<Alert> out;
    auto stmt = db.prepare(
        "SELECT id,printer_id,severity,type,message,acknowledged,created_at FROM printer_alerts "
        "WHERE resolved_at=0 ORDER BY created_at DESC"
    );
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Alert a;
        a.id           = sqlite3_column_int(stmt, 0);
        a.printer_id   = sqlite3_column_int(stmt, 1);
        a.severity     = col_text(stmt, 2);
        a.type         = col_text(stmt, 3);
        a.message      = col_text(stmt, 4);
        a.acknowledged = sqlite3_column_int(stmt, 5) != 0;
        a.created_at   = sqlite3_column_int64(stmt, 6);
        out.push_back(a);
    }
    db.finalize(stmt);
    return out;
}

void Repository::save_jobs(const std::vector<PrintJob>& jobs) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    long long ts = now_epoch();
    db.begin();
    try {
        if (!jobs.empty()) {
            auto del = db.prepare("DELETE FROM printer_jobs WHERE printer_id=?");
            sqlite3_bind_int(del, 1, jobs[0].printer_id);
            db.step(del);
            db.finalize(del);
        }
        for (auto& j : jobs) {
            auto stmt = db.prepare(
                "INSERT INTO printer_jobs(printer_id,job_id,job_name,user_name,status,pages,created_at) "
                "VALUES(?,?,?,?,?,?,?)"
            );
            sqlite3_bind_int(stmt,  1, j.printer_id);
            sqlite3_bind_int(stmt,  2, j.job_id);
            sqlite3_bind_text(stmt, 3, j.job_name.c_str(),  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, j.user_name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, j.status.c_str(),    -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt,  6, j.pages);
            sqlite3_bind_int64(stmt,7, j.created_at ? j.created_at : ts);
            db.step(stmt);
            db.finalize(stmt);
        }
        db.commit();
    } catch (...) {
        db.rollback();
        throw;
    }
}

std::vector<PrintJob> Repository::latest_jobs(int printer_id, int limit) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    limit = std::max(1, std::min(limit, 200));
    auto stmt = db.prepare("SELECT printer_id,job_id,job_name,user_name,status,pages,created_at FROM printer_jobs WHERE printer_id=? ORDER BY created_at DESC,id DESC LIMIT ?");
    sqlite3_bind_int(stmt, 1, printer_id);
    sqlite3_bind_int(stmt, 2, limit);
    std::vector<PrintJob> out;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PrintJob j;
        j.printer_id = sqlite3_column_int(stmt, 0);
        j.job_id = sqlite3_column_int(stmt, 1);
        j.job_name = col_text(stmt, 2);
        j.user_name = col_text(stmt, 3);
        j.status = col_text(stmt, 4);
        j.pages = sqlite3_column_int(stmt, 5);
        j.created_at = sqlite3_column_int64(stmt, 6);
        out.push_back(std::move(j));
    }
    db.finalize(stmt);
    return out;
}

void Repository::save_event(int printer_id, const std::string& type, const std::string& message, long long ts) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare("INSERT INTO printer_events(printer_id,type,message,created_at) VALUES(?,?,?,?)");
    sqlite3_bind_int(stmt, 1, printer_id);
    sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, ts ? ts : now_epoch());
    db.step(stmt);
    db.finalize(stmt);
}

void Repository::save_log(int printer_id, const std::string& source, const std::string& message, long long ts) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare("INSERT INTO printer_logs(printer_id,source,message,created_at) VALUES(?,?,?,?)");
    if (printer_id > 0) sqlite3_bind_int(stmt, 1, printer_id);
    else sqlite3_bind_null(stmt, 1);
    sqlite3_bind_text(stmt, 2, source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, ts ? ts : now_epoch());
    db.step(stmt);
    db.finalize(stmt);
}

std::vector<PrinterEvent> Repository::latest_events(int printer_id, int limit) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    limit = std::max(1, std::min(limit, 1000));
    auto stmt = db.prepare("SELECT id,printer_id,type,message,created_at FROM printer_events WHERE printer_id=? ORDER BY created_at DESC,id DESC LIMIT ?");
    sqlite3_bind_int(stmt, 1, printer_id);
    sqlite3_bind_int(stmt, 2, limit);
    std::vector<PrinterEvent> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PrinterEvent event;
        event.id = sqlite3_column_int(stmt, 0);
        event.printer_id = sqlite3_column_int(stmt, 1);
        event.type = col_text(stmt, 2);
        event.message = col_text(stmt, 3);
        event.created_at = sqlite3_column_int64(stmt, 4);
        result.push_back(std::move(event));
    }
    db.finalize(stmt);
    return result;
}

std::vector<PrinterLog> Repository::latest_logs(int printer_id, int limit) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    limit = std::max(1, std::min(limit, 1000));
    auto stmt = db.prepare("SELECT id,COALESCE(printer_id,0),source,message,created_at FROM printer_logs WHERE printer_id=? ORDER BY created_at DESC,id DESC LIMIT ?");
    sqlite3_bind_int(stmt, 1, printer_id);
    sqlite3_bind_int(stmt, 2, limit);
    std::vector<PrinterLog> result;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PrinterLog log;
        log.id = sqlite3_column_int(stmt, 0);
        log.printer_id = sqlite3_column_int(stmt, 1);
        log.source = col_text(stmt, 2);
        log.message = col_text(stmt, 3);
        log.created_at = sqlite3_column_int64(stmt, 4);
        result.push_back(std::move(log));
    }
    db.finalize(stmt);
    return result;
}

void Repository::record_history(int printer_id, const std::string& metric, double value, long long ts) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    auto stmt = db.prepare(
        "INSERT INTO printer_history(printer_id,metric,value,recorded_at) VALUES(?,?,?,?)"
    );
    sqlite3_bind_int(stmt,  1, printer_id);
    sqlite3_bind_text(stmt, 2, metric.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt,3, value);
    sqlite3_bind_int64(stmt,4, ts ? ts : now_epoch());
    db.step(stmt);
    db.finalize(stmt);
}

std::vector<std::pair<long long,double>> Repository::history(int printer_id, const std::string& metric, int limit) {
    std::lock_guard<std::recursive_mutex> lock(mu);
    limit = std::max(1, std::min(limit, 5000));
    std::vector<std::pair<long long,double>> out;
    auto stmt = db.prepare(
        "SELECT recorded_at, value FROM printer_history "
        "WHERE printer_id=? AND metric=? ORDER BY recorded_at DESC LIMIT ?"
    );
    sqlite3_bind_int(stmt,  1, printer_id);
    sqlite3_bind_text(stmt, 2, metric.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  3, limit);
    int rc;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        out.emplace_back(sqlite3_column_int64(stmt, 0), sqlite3_column_double(stmt, 1));
    }
    db.finalize(stmt);
    return out;
}
