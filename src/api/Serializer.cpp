#include "Serializer.hpp"

namespace ApiSerializer {
using nlohmann::json;

json printer(const Printer& p) {
    return {{"id", p.id}, {"connection_type", p.connection_type}, {"endpoint", p.ip},
            {"port_name", p.port_name}, {"ip", p.connection_type == "network" ? p.ip : ""},
            {"hostname", p.hostname}, {"mac", p.mac}, {"manufacturer", p.manufacturer},
            {"model", p.model}, {"friendly_name", p.friendly_name}, {"serial", p.serial},
            {"firmware", p.fw_version}, {"is_active", p.is_active}, {"last_seen", p.last_seen},
            {"response_ms", p.response_ms}, {"has_duplex", p.has_duplex},
            {"has_scanner", p.has_scanner}, {"has_fax", p.has_fax}, {"tray_count", p.tray_count}};
}

json status(const PrinterStatus& s) {
    return {{"printer_id", s.printer_id}, {"status", s.status}, {"message", s.status_message},
            {"error_code", s.error_code}, {"uptime_s", s.uptime_s}, {"collected_at", s.collected_at}};
}

json consumable(const Consumable& c) {
    return {{"name", c.name}, {"type", c.type}, {"color", c.color},
            {"capacity_pages", c.capacity_pages}, {"remaining_pages", c.remaining_pages},
            {"level_pct", c.level_pct}, {"state", c.state}, {"collected_at", c.collected_at}};
}

json counter(const Counter& c) {
    return {{"total", c.total_pages}, {"mono", c.mono_pages}, {"color", c.color_pages},
            {"simplex", c.simplex_pages}, {"duplex", c.duplex_pages}, {"copies", c.copy_pages},
            {"prints", c.print_pages}, {"scans", c.scan_pages}, {"fax", c.fax_pages},
            {"adf", c.adf_pages}, {"usb", c.usb_pages}, {"network", c.net_pages},
            {"secure", c.secure_pages}, {"collected_at", c.collected_at}};
}

json alert(const Alert& a) {
    return {{"id", a.id}, {"printer_id", a.printer_id}, {"severity", a.severity},
            {"type", a.type}, {"message", a.message}, {"acknowledged", a.acknowledged},
            {"created_at", a.created_at}, {"resolved_at", a.resolved_at}};
}

json network(const NetworkConfig& n) {
    return {{"ipv4", n.ipv4}, {"ipv6", n.ipv6}, {"gateway", n.gateway}, {"dns", n.dns},
            {"subnet", n.subnet}, {"dhcp", n.dhcp}, {"speed_mbps", n.speed_mbps},
            {"uptime_s", n.uptime_s}, {"iface_type", n.iface_type}, {"collected_at", n.collected_at}};
}

json security(const SecurityInfo& s) {
    return {{"snmp_version", s.snmp_version}, {"snmp_community", s.snmp_community},
            {"http_enabled", s.http_enabled}, {"https_enabled", s.https_enabled},
            {"telnet_enabled", s.telnet_enabled}, {"ftp_enabled", s.ftp_enabled},
            {"ssh_enabled", s.ssh_enabled}, {"smb_enabled", s.smb_enabled},
            {"tls_version", s.tls_version}, {"cert_expiry", s.cert_expiry},
            {"default_password", s.default_password}, {"open_ports", s.open_ports},
            {"collected_at", s.collected_at}};
}

json job(const PrintJob& j) {
    return {{"printer_id", j.printer_id}, {"job_id", j.job_id}, {"name", j.job_name},
            {"user", j.user_name}, {"status", j.status}, {"pages", j.pages}, {"created_at", j.created_at}};
}

json event(const PrinterEvent& event) {
    return {{"id", event.id}, {"printer_id", event.printer_id}, {"type", event.type},
            {"message", event.message}, {"created_at", event.created_at}};
}

json log(const PrinterLog& log) {
    return {{"id", log.id}, {"printer_id", log.printer_id}, {"source", log.source},
            {"message", log.message}, {"created_at", log.created_at}};
}

json error(const std::string& message) { return {{"error", message}}; }
}
