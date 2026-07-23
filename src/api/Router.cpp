#include "Router.hpp"
#include "Serializer.hpp"

#include <chrono>
#include <algorithm>

namespace {
bool authorized(const httplib::Request& request, const ApiConfig& config) {
    if (config.token.empty()) return false;
    return request.get_header_value("Authorization") == "Bearer " + config.token;
}

bool require_auth(const httplib::Request& request, httplib::Response& response, const ApiConfig& config) {
    if (authorized(request, config)) return true;
    response.status = 401;
    response.set_content(ApiSerializer::error("unauthorized").dump(), "application/json");
    return false;
}

void no_data(httplib::Response& response) {
    response.status = 404;
    response.set_content(ApiSerializer::error("no data").dump(), "application/json");
}

int bounded_limit(const httplib::Request& request, int fallback = 100) {
    if (!request.has_param("limit")) return fallback;
    try { return std::max(1, std::min(std::stoi(request.get_param_value("limit")), 1000)); }
    catch (...) { return fallback; }
}
}

void Router::register_routes(httplib::Server& server, Repository& repo, const ApiConfig& cfg,
                             std::function<void()> trigger_discovery) {
    server.Get("/api/v1/health", [](const httplib::Request&, httplib::Response& response) {
        const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        response.set_content(nlohmann::json{{"status", "ok"}, {"ts", now}}.dump(), "application/json");
    });

    server.Get("/api/v1/printers", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        nlohmann::json result = nlohmann::json::array();
        for (const auto& printer : repo.all_printers()) result.push_back(ApiSerializer::printer(printer));
        response.set_content(result.dump(), "application/json");
    });

    server.Get(R"(/api/v1/printers/(\d+))", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        const auto printer = repo.find_printer(std::stoi(request.matches[1]));
        if (!printer) return no_data(response);
        response.set_content(ApiSerializer::printer(*printer).dump(), "application/json");
    });

    server.Get(R"(/api/v1/printers/(\d+)/status)", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        const auto value = repo.latest_status(std::stoi(request.matches[1]));
        if (!value) return no_data(response);
        response.set_content(ApiSerializer::status(*value).dump(), "application/json");
    });
    server.Get(R"(/api/v1/printers/(\d+)/consumables)", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        nlohmann::json result = nlohmann::json::array();
        for (const auto& value : repo.latest_consumables(std::stoi(request.matches[1]))) result.push_back(ApiSerializer::consumable(value));
        response.set_content(result.dump(), "application/json");
    });
    server.Get(R"(/api/v1/printers/(\d+)/counters)", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        const auto value = repo.latest_counter(std::stoi(request.matches[1]));
        if (!value) return no_data(response);
        response.set_content(ApiSerializer::counter(*value).dump(), "application/json");
    });
    server.Get(R"(/api/v1/printers/(\d+)/network)", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        const auto value = repo.latest_network(std::stoi(request.matches[1]));
        if (!value) return no_data(response);
        response.set_content(ApiSerializer::network(*value).dump(), "application/json");
    });
    server.Get(R"(/api/v1/printers/(\d+)/security)", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        const auto value = repo.latest_security(std::stoi(request.matches[1]));
        if (!value) return no_data(response);
        response.set_content(ApiSerializer::security(*value).dump(), "application/json");
    });
    server.Get(R"(/api/v1/printers/(\d+)/jobs)", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        nlohmann::json result = nlohmann::json::array();
        for (const auto& value : repo.latest_jobs(std::stoi(request.matches[1]), bounded_limit(request, 20))) result.push_back(ApiSerializer::job(value));
        response.set_content(result.dump(), "application/json");
    });
    server.Get(R"(/api/v1/printers/(\d+)/alerts)", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        nlohmann::json result = nlohmann::json::array();
        for (const auto& value : repo.open_alerts(std::stoi(request.matches[1]))) result.push_back(ApiSerializer::alert(value));
        response.set_content(result.dump(), "application/json");
    });
    server.Get(R"(/api/v1/printers/(\d+)/events)", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        nlohmann::json result = nlohmann::json::array();
        for (const auto& value : repo.latest_events(std::stoi(request.matches[1]), bounded_limit(request))) result.push_back(ApiSerializer::event(value));
        response.set_content(result.dump(), "application/json");
    });
    server.Get(R"(/api/v1/printers/(\d+)/logs)", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        nlohmann::json result = nlohmann::json::array();
        for (const auto& value : repo.latest_logs(std::stoi(request.matches[1]), bounded_limit(request))) result.push_back(ApiSerializer::log(value));
        response.set_content(result.dump(), "application/json");
    });
    server.Get(R"(/api/v1/printers/(\d+)/history)", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        if (!request.has_param("metric")) {
            response.status = 400;
            response.set_content(ApiSerializer::error("metric is required").dump(), "application/json");
            return;
        }
        nlohmann::json result = nlohmann::json::array();
        for (const auto& [ts, value] : repo.history(std::stoi(request.matches[1]), request.get_param_value("metric"), bounded_limit(request))) {
            result.push_back({{"ts", ts}, {"value", value}});
        }
        response.set_content(result.dump(), "application/json");
    });
    server.Get("/api/v1/alerts", [&repo, cfg](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        nlohmann::json result = nlohmann::json::array();
        for (const auto& value : repo.all_open_alerts()) result.push_back(ApiSerializer::alert(value));
        response.set_content(result.dump(), "application/json");
    });
    server.Post("/api/v1/printers/discovery", [cfg, trigger_discovery](const httplib::Request& request, httplib::Response& response) {
        if (!require_auth(request, response, cfg)) return;
        if (!trigger_discovery) {
            response.status = 503;
            response.set_content(ApiSerializer::error("discovery unavailable").dump(), "application/json");
            return;
        }
        trigger_discovery();
        response.status = 202;
        response.set_content(nlohmann::json{{"status", "scheduled"}}.dump(), "application/json");
    });
}
