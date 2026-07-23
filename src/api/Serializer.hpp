#pragma once

#include "../models/Alert.hpp"
#include "../models/Consumable.hpp"
#include "../models/Counter.hpp"
#include "../models/Event.hpp"
#include "../models/NetworkConfig.hpp"
#include "../models/Printer.hpp"
#include <nlohmann/json.hpp>

namespace ApiSerializer {
nlohmann::json printer(const Printer& value);
nlohmann::json status(const PrinterStatus& value);
nlohmann::json consumable(const Consumable& value);
nlohmann::json counter(const Counter& value);
nlohmann::json alert(const Alert& value);
nlohmann::json network(const NetworkConfig& value);
nlohmann::json security(const SecurityInfo& value);
nlohmann::json job(const PrintJob& value);
nlohmann::json event(const PrinterEvent& value);
nlohmann::json log(const PrinterLog& value);
nlohmann::json error(const std::string& message);
}
