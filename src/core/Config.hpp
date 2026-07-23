#pragma once
#include <string>
#include <vector>

struct SnmpConfig {
    std::string version  = "2c";
    std::string community = "public";
    std::string auth_proto = "MD5";   
    std::string priv_proto = "DES";
    std::string auth_pass  = "";
    std::string priv_pass  = "";
    std::string sec_name   = "";
    int timeout  = 3;
    int retries  = 2;
    int port     = 161;
};

struct DiscoveryConfig {
    std::vector<std::string> ip_ranges;
    std::vector<int> ports = {161, 631, 9100, 80, 443};
    bool mdns    = true;
    bool arp     = true;
    bool usb     = true;
    int threads  = 64;      
    int timeout_ms = 1500;
};

struct DatabaseConfig {
    std::string path = "agent.db";
};

struct ApiConfig {
    std::string host  = "0.0.0.0";
    int port          = 8080;
    std::string token = "";    
};

struct AlertThresholds {
    int toner_warning  = 20;
    int toner_critical = 5;
    int offline_minutes = 5;
};

struct WebhookConfig {
    std::string url;
    bool enabled = false;
};

struct SmtpConfig {
    std::string host;
    int port = 587;
    std::string from;
    std::vector<std::string> to;
    std::string user;
    std::string pass;
    bool enabled = false;
};

struct SyslogConfig {
    std::string host = "127.0.0.1";
    int port = 5514;
    bool enabled = false;
};

struct AgentConfig {
    std::string name            = "santa-agent";
    int collection_interval     = 300;   
    int discovery_interval      = 3600;
    int thread_pool_size        = 8;
    std::string log_level       = "info";
    std::string log_file        = "santa-agent.log";

    SnmpConfig      snmp;
    DiscoveryConfig discovery;
    DatabaseConfig  database;
    ApiConfig       api;
    AlertThresholds alert_thresholds;
    WebhookConfig   webhook;
    SmtpConfig      smtp;
    SyslogConfig    syslog;
};

class Config {
public:
    static Config& get() {
        static Config inst;
        return inst;
    }

    bool load(const std::string& path);
    const AgentConfig& data() const { return cfg; }

private:
    Config() = default;
    AgentConfig cfg;
};
