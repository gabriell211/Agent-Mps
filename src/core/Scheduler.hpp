#pragma once
#include <functional>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>

struct ScheduledTask {
    std::string           name;
    std::function<void()> fn;
    int                   interval_s;    // em segundos
    std::chrono::steady_clock::time_point next_run;
};

class Scheduler {
public:
    Scheduler() = default;
    ~Scheduler() { stop(); }

    void add(const std::string& name, std::function<void()> fn, int interval_s, bool run_now = true);
    void start();
    void stop();

private:
    std::vector<ScheduledTask> tasks;
    std::thread                worker;
    std::atomic<bool>          running{false};
    std::mutex                 mu;
};
