#pragma once
#include <string>
#include <unordered_map>
#include <shared_mutex>

class ThreadSafeDB {
private:
    std::unordered_map<std::string, std::string> data;
    mutable std::shared_timed_mutex rw_mutex;

public:
    void set(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value) const;
    void erase(const std::string& key);
};
