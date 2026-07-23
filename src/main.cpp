#include "core/Agent.hpp"
#include <iostream>
#include <csignal>
#include <cstring>

static Agent* g_agent = nullptr;

static void sig_handler(int sig) {
    if (g_agent) g_agent->stop();
}

static void print_help(const char* name) {
    std::cout << "uso: " << name << " [opcoes]\n"
              << "  --config <arquivo>   arquivo de configuracao (padrao: config.toml)\n"
              << "  --help               mostra esta ajuda\n"
              << "  --version            exibe versao\n";
}

int main(int argc, char** argv) {
    std::string config_file = "config.toml";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--version") == 0) {
            std::cout << "santa-agent 1.0.0\n";
            return 0;
        }
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            config_file = argv[++i];
        }
    }

    signal(SIGTERM, sig_handler);
    signal(SIGINT,  sig_handler);

    Agent agent;
    g_agent = &agent;

    if (!agent.init(config_file)) {
        std::cerr << "falha na inicializacao. verifique o config e os logs.\n";
        return 1;
    }

    agent.run();
    return 0;
}
