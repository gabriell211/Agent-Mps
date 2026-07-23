#pragma once
#include <string>
#include <vector>

struct NetworkConfig {
    int         printer_id    = 0;
    std::string ipv4;
    std::string ipv6;
    std::string gateway;
    std::string dns;          
    std::string subnet;
    bool        dhcp          = false;
    int         speed_mbps    = 0;   
    long long   uptime_s      = 0;
    std::string iface_type;  
    long long   collected_at  = 0;
};

struct SecurityInfo {
    int         printer_id       = 0;
    std::string snmp_version;    
    std::string snmp_community;
    bool        http_enabled     = false;
    bool        https_enabled    = false;
    bool        telnet_enabled   = false;
    bool        ftp_enabled      = false;
    bool        ssh_enabled      = false;
    bool        smb_enabled      = false;
    std::string tls_version;
    std::string cert_expiry;    
    bool        default_password = false;
    std::vector<int> open_ports;
    long long   collected_at     = 0;
};

struct PrintJob {
    int         printer_id  = 0;
    int         job_id      = 0;
    std::string job_name;
    std::string user_name;
    std::string status;      
    int         pages       = 0;
    long long   created_at  = 0;
};
