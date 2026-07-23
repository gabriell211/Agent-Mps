#pragma once
#include "../storage/Repository.hpp"
#include "../core/Config.hpp"
#include <httplib.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

class RestServer {
public:
    RestServer(Repository& repo, const ApiConfig& cfg,
               std::function<void()> trigger_discovery = {});
    ~RestServer();

    void start();   
    void stop();

private:
    Repository& repo;
    ApiConfig   cfg;
    std::thread worker;
    std::atomic<bool> running{false};
    std::function<void()> trigger_discovery;
    std::unique_ptr<httplib::Server> server;
    std::mutex server_mu;

    bool auth(const std::string& token_header);

    void setup_routes(httplib::Server& server);
};
