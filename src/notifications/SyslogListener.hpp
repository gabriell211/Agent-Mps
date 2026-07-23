#pragma once

#include "../core/Config.hpp"
#include <atomic>
#include <functional>
#include <thread>

class SyslogListener {
public:
    explicit SyslogListener(const SyslogConfig& cfg) : cfg(cfg) {}
    ~SyslogListener() { stop(); }

    void start(std::function<void(const std::string&, const std::string&)> callback = {});
    void stop();

private:
    SyslogConfig cfg;
    std::function<void(const std::string&, const std::string&)> callback;
    std::atomic<bool> running{false};
    std::thread worker;
};
