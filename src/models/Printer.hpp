#pragma once
#include <string>
#include <chrono>

struct Printer {
    int         id          = 0;
    std::string connection_type = "network";
    std::string ip;
    std::string port_name;
    std::string hostname;
    std::string mac;
    std::string manufacturer;
    std::string model;
    std::string friendly_name;
    std::string serial;
    std::string asset_tag;
    std::string uuid;

    int         memory_mb     = 0;
    std::string cpu;
    std::string flash;
    bool        has_duplex    = false;
    bool        has_scanner   = false;
    bool        has_fax       = false;
    bool        has_stapler   = false;
    int         tray_count    = 0;

    std::string fw_version;
    std::string fw_build;
    std::string fw_date;
    std::string fw_engine;
    std::string bootloader;

    bool        is_active     = true;
    long long   discovered_at = 0;   // epoch
    long long   last_seen     = 0;
    int         response_ms   = 0;
};

struct PrinterStatus {
    int         printer_id = 0;
    std::string status;          
    std::string status_message;
    std::string error_code;
    long long   uptime_s   = 0;
    long long   collected_at = 0;
};
