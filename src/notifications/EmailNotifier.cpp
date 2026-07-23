#include "EmailNotifier.hpp"
#include "../core/Logger.hpp"

#include <curl/curl.h>
#include <algorithm>
#include <cstring>
#include <sstream>

namespace {
struct UploadBody {
    std::string data;
    size_t offset = 0;
};

size_t read_mail(char* buffer, size_t size, size_t count, void* user_data) {
    auto* body = static_cast<UploadBody*>(user_data);
    const size_t capacity = size * count;
    const size_t remaining = body->data.size() - body->offset;
    const size_t length = std::min(capacity, remaining);
    if (length) {
        std::memcpy(buffer, body->data.data() + body->offset, length);
        body->offset += length;
    }
    return length;
}

std::string sanitize_header(std::string value) {
    value.erase(std::remove(value.begin(), value.end(), '\r'), value.end());
    value.erase(std::remove(value.begin(), value.end(), '\n'), value.end());
    return value;
}
}

void EmailNotifier::notify(const Alert& alert, const Printer& printer) const {
    if (!cfg.enabled || cfg.host.empty() || cfg.from.empty() || cfg.to.empty()) return;

    const std::string subject = sanitize_header("[Santa Agent][" + alert.severity + "] " + alert.type);
    const std::string endpoint = printer.connection_type == "network" ? printer.ip : printer.port_name;
    UploadBody body;
    body.data = "From: " + sanitize_header(cfg.from) + "\r\n"
                "To: " + sanitize_header(cfg.to.front()) + "\r\n"
                "Subject: " + subject + "\r\n"
                "Content-Type: text/plain; charset=UTF-8\r\n\r\n"
                "Impressora: " + printer.model + "\r\n"
                "Endpoint: " + endpoint + "\r\n"
                "Alerta: " + alert.message + "\r\n";

    CURL* curl = curl_easy_init();
    if (!curl) return;
    const std::string url = "smtp://" + cfg.host + ":" + std::to_string(cfg.port);
    curl_slist* recipients = nullptr;
    for (const auto& address : cfg.to) recipients = curl_slist_append(recipients, address.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USE_SSL, static_cast<long>(CURLUSESSL_ALL));
    curl_easy_setopt(curl, CURLOPT_USERNAME, cfg.user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, cfg.pass.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, cfg.from.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_mail);
    curl_easy_setopt(curl, CURLOPT_READDATA, &body);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    const CURLcode result = curl_easy_perform(curl);
    if (result != CURLE_OK) Log::error("email: falha ao enviar: {}", curl_easy_strerror(result));
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
}
