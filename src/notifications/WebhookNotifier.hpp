#pragma once
#include "../core/Config.hpp"
#include "../models/Alert.hpp"
#include "../models/Printer.hpp"
#include <string>

class WebhookNotifier {
public:
    explicit WebhookNotifier(const WebhookConfig& cfg);
    void notify(const Alert& alert, const Printer& printer);

private:
    WebhookConfig cfg;
    std::string build_payload(const Alert& alert, const Printer& printer);
};
