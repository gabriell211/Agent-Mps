#pragma once

#include "../core/Config.hpp"
#include "../storage/Repository.hpp"
#include <httplib.h>
#include <functional>

class Router {
public:
    static void register_routes(httplib::Server& server, Repository& repository,
                                const ApiConfig& config,
                                std::function<void()> trigger_discovery = {});
};
