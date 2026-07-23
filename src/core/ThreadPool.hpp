#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(int n);
    ~ThreadPool();

    void submit(std::function<void()> task);
    void wait();  

private:
    std::vector<std::thread>          workers;
    std::queue<std::function<void()>> tasks;
    std::mutex                        mu;
    std::condition_variable           cv;
    std::condition_variable           done_cv;
    std::atomic<int>                  active{0};
    bool                              stop_flag = false;
};
