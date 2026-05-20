#include "eviction.hpp"

bool Expiration::operator>(const Expiration& other) const {
    return expire_at > other.expire_at;
}

TTLEngine::TTLEngine(ThreadSafeDB& db) : db(db), running(true) {
    worker = std::thread(&TTLEngine::backgroundLoop, this);
}

TTLEngine::~TTLEngine() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
    }
    cv.notify_all();
    if (worker.joinable()) {
        worker.join();
    }
}

void TTLEngine::addTTL(const std::string& key, int seconds) {
    auto expire_at = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
    std::lock_guard<std::mutex> lock(mtx);
    min_heap.push({key, expire_at});
    cv.notify_one();
}

void TTLEngine::backgroundLoop() {
    while (running) {
        std::unique_lock<std::mutex> lock(mtx);
        if (min_heap.empty()) {
            cv.wait(lock, [this]() { return !running || !min_heap.empty(); });
        } else {
            auto now = std::chrono::steady_clock::now();
            if (min_heap.top().expire_at <= now) {
                std::string key = min_heap.top().key;
                min_heap.pop();
                lock.unlock(); // Unlock before interacting with DB to prevent deadlocks
                db.erase(key);
            } else {
                cv.wait_until(lock, min_heap.top().expire_at, [this]() { return !running; });
            }
        }
    }
}
