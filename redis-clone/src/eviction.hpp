#pragma once
#include "storage.hpp"
#include <string>
#include <queue>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>

struct Expiration {
    std::string key;
    std::chrono::steady_clock::time_point expire_at;

    bool operator>(const Expiration& other) const;
};

class TTLEngine {
private:
    std::priority_queue<Expiration, std::vector<Expiration>, std::greater<Expiration>> min_heap;
    std::mutex mtx;
    std::condition_variable cv;
    ThreadSafeDB& db;
    bool running;
    std::thread worker;

    void backgroundLoop();

public:
    TTLEngine(ThreadSafeDB& db);
    ~TTLEngine();
    void addTTL(const std::string& key, int seconds);
};
