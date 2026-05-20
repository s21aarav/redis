#include "storage.hpp"

void ThreadSafeDB::set(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex);
    data[key] = value;
}

bool ThreadSafeDB::get(const std::string& key, std::string& value) const {
    std::shared_lock<std::shared_mutex> lock(rw_mutex);
    auto it = data.find(key);
    if (it != data.end()) {
        value = it->second;
        return true;
    }
    return false;
}

void ThreadSafeDB::erase(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex);
    data.erase(key);
}
