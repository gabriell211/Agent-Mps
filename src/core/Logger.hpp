#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <string>

class Log {
public:
    static void init(const std::string& level, const std::string& file, size_t max_mb = 10, int keep = 3);

    static spdlog::logger* get() { return logger.get(); }

    template<typename... Args>
    static void trace(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger) logger->trace(fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger) logger->debug(fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger) logger->info(fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger) logger->warn(fmt, std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        if (logger) logger->error(fmt, std::forward<Args>(args)...);
    }

private:
    static std::shared_ptr<spdlog::logger> logger;
};
