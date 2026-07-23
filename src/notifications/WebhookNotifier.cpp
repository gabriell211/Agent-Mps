#include "WebhookNotifier.hpp"
#include "../core/Logger.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>

WebhookNotifier::WebhookNotifier(const WebhookConfig& cfg) : cfg(cfg) {}

std::string WebhookNotifier::build_payload(const Alert& alert, const Printer& printer) {
    std::string color = alert.severity == "critical" ? "#FF0000"
                      : alert.severity == "warning"  ? "#FFA500"
                      : "#00AA00";
    std::string icon  = alert.severity == "critical" ? ":red_circle:"
                      : alert.severity == "warning"  ? ":warning:"
                      : ":white_check_mark:";

    const std::string endpoint = printer.connection_type == "network" ? printer.ip : printer.port_name;
    nlohmann::json fields = nlohmann::json::array({
        {{"title", "Impressora"}, {"value", printer.model}, {"short", true}},
        {{"title", "Endpoint"}, {"value", endpoint}, {"short", true}},
        {{"title", "Tipo"}, {"value", alert.type}, {"short", true}},
        {{"title", "Severidade"}, {"value", alert.severity}, {"short", true}},
        {{"title", "Mensagem"}, {"value", alert.message}, {"short", false}}
    });
    nlohmann::json attachment = {{"color", color}, {"fields", fields}};
    nlohmann::json payload = {
        {"text", icon + " *[" + alert.severity + "]* " + printer.model + " (" + endpoint + "): " + alert.message},
        {"attachments", nlohmann::json::array({attachment})}
    };
    return payload.dump();
}

void WebhookNotifier::notify(const Alert& alert, const Printer& printer) {
    if (!cfg.enabled || cfg.url.empty()) return;

    std::string payload = build_payload(alert, printer);
    CURL* curl = curl_easy_init();
    if (!curl) return;

    struct curl_slist* hdrs = nullptr;
    hdrs = curl_slist_append(hdrs, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, cfg.url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK)
        Log::error("webhook: falha ao enviar: {}", curl_easy_strerror(rc));
    else
        Log::debug("webhook: notificacao enviada para {}", printer.ip);

    curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
}
