#include "ThreadPool.hpp"
#include "Logger.hpp"

ThreadPool::ThreadPool(int n) {
    for (int i = 0; i < n; i++) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lk(mu);
                    cv.wait(lk, [this] { return stop_flag || !tasks.empty(); });
                    if (stop_flag && tasks.empty()) return;
                    task = std::move(tasks.front());
                    tasks.pop();
                    active++;
                }
                try {
                    task();
                } catch (const std::exception& error) {
                    Log::error("thread-pool: tarefa falhou: {}", error.what());
                } catch (...) {
                    Log::error("thread-pool: tarefa falhou com erro desconhecido");
                }
                {
                    std::lock_guard<std::mutex> lk(mu);
                    active--;
                }
                done_cv.notify_all();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lk(mu);
        stop_flag = true;
    }
    cv.notify_all();
    for (auto& w : workers) w.join();
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lk(mu);
        tasks.push(std::move(task));
    }
    cv.notify_one();
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lk(mu);
    done_cv.wait(lk, [this] { return tasks.empty() && active.load() == 0; });
}
