#include "Scheduler.hpp"
#include "Logger.hpp"

void Scheduler::add(const std::string& name, std::function<void()> fn, int interval_s, bool run_now) {
    ScheduledTask t;
    t.name       = name;
    t.fn         = std::move(fn);
    t.interval_s = interval_s;
    t.next_run   = run_now
        ? std::chrono::steady_clock::now()
        : std::chrono::steady_clock::now() + std::chrono::seconds(interval_s);
    std::lock_guard<std::mutex> lk(mu);
    tasks.push_back(std::move(t));
}

void Scheduler::start() {
    running = true;
    worker  = std::thread([this] {
        while (running) {
            auto now = std::chrono::steady_clock::now();
            {
                std::lock_guard<std::mutex> lk(mu);
                for (auto& t : tasks) {
                    if (now >= t.next_run) {
                        Log::debug("scheduler: rodando '{}'", t.name);
                        try {
                            t.fn();
                        } catch (const std::exception& e) {
                            Log::error("scheduler: erro em '{}': {}", t.name, e.what());
                        }
                        t.next_run = now + std::chrono::seconds(t.interval_s);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
}

void Scheduler::stop() {
    running = false;
    if (worker.joinable()) worker.join();
}
