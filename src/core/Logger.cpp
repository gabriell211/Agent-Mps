#include "Logger.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>

std::shared_ptr<spdlog::logger> Log::logger;

void Log::init(const std::string& level, const std::string& file, size_t max_mb, int keep) {
    std::vector<spdlog::sink_ptr> sinks;
    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sinks.push_back(console);
    if (!file.empty()) {
        auto fsink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            file, max_mb * 1024 * 1024, keep
        );
        sinks.push_back(fsink);
    }

    logger = std::make_shared<spdlog::logger>("agent", sinks.begin(), sinks.end());
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

    if (level == "trace")      logger->set_level(spdlog::level::trace);
    else if (level == "debug") logger->set_level(spdlog::level::debug);
    else if (level == "warn")  logger->set_level(spdlog::level::warn);
    else if (level == "error") logger->set_level(spdlog::level::err);
    else                       logger->set_level(spdlog::level::info);

    spdlog::register_logger(logger);
    spdlog::flush_every(std::chrono::seconds(5));
}
