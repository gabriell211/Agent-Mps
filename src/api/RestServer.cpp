#include "RestServer.hpp"
#include "Router.hpp"
#include "../core/Logger.hpp"

RestServer::RestServer(Repository& repo, const ApiConfig& cfg,
                       std::function<void()> trigger_discovery)
    : repo(repo), cfg(cfg), trigger_discovery(std::move(trigger_discovery)) {}

RestServer::~RestServer() { stop(); }

bool RestServer::auth(const std::string& token_header) {
    return !cfg.token.empty() && token_header == "Bearer " + cfg.token;
}

void RestServer::setup_routes(httplib::Server& instance) {
    Router::register_routes(instance, repo, cfg, trigger_discovery);
}

void RestServer::start() {
    std::lock_guard<std::mutex> lock(server_mu);
    if (running || worker.joinable()) return;
    if (cfg.token.empty()) {
        Log::error("api: token vazio; a API nao sera iniciada");
        return;
    }
    server = std::make_unique<httplib::Server>();
    setup_routes(*server);
    running = true;
    worker = std::thread([this] {
        Log::info("api: escutando em {}:{}", cfg.host, cfg.port);
        const bool listened = server->listen(cfg.host.c_str(), cfg.port);
        if (!listened && running) Log::error("api: nao foi possivel escutar em {}:{}", cfg.host, cfg.port);
        running = false;
    });
}

void RestServer::stop() {
    {
        std::lock_guard<std::mutex> lock(server_mu);
        running = false;
        if (server) server->stop();
    }
    if (worker.joinable()) worker.join();
    std::lock_guard<std::mutex> lock(server_mu);
    server.reset();
}
