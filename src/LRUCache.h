#pragma once

#include <list>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>
#include <stdexcept>

// ─────────────────────────────────────────────────────────
//  LRUCache<K, V>
//
//  A thread-safe, generic Least Recently Used cache.
//
//  Data structure:
//    list<{key, value, expiry}>          — recency order, MRU at front
//    unordered_map<key, list::iterator>  — O(1) lookup into the list
//
//  Complexity:
//    get()  — O(1) average
//    put()  — O(1) average
//    erase()— O(1) average
//
//  Features:
//    - Templated key/value types
//    - Thread safety via std::mutex
//    - Optional TTL (time-to-live) per entry
//    - Hit/miss statistics
// ─────────────────────────────────────────────────────────

template<typename K, typename V>
class LRUCache {
public:
    // ttl_seconds = 0 means no expiry
    explicit LRUCache(int capacity, int ttl_seconds = 0);

    // Returns value if key exists and is not expired
    std::optional<V> get(const K& key);

    // Insert or update a key-value pair
    void put(const K& key, const V& value);

    // Remove a key explicitly
    bool erase(const K& key);

    // Check if key exists (and is not expired)
    bool contains(const K& key);

    // Current number of entries
    int size() const;

    // Max capacity
    int capacity() const;

    // Stats
    long long hits() const;
    long long misses() const;
    double hitRate() const;
    void resetStats();

    // Print current cache state (MRU → LRU)
    void display() const;

private:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    struct Entry {
        K         key;
        V         value;
        TimePoint expiry;   // zero-initialised = no expiry
        bool      hasTTL;
    };

    using ListIt = typename std::list<Entry>::iterator;

    int                              capacity_;
    int                              ttlSeconds_;
    std::list<Entry>                 cacheList_;   // front = MRU, back = LRU
    std::unordered_map<K, ListIt>    cacheMap_;
    mutable std::mutex               mutex_;

    // Stats
    long long hits_   = 0;
    long long misses_ = 0;

    // Internal helpers (call with mutex already held)
    bool isExpired(const Entry& e) const;
    void evictExpired();                  // lazy expiry sweep
    void removeLRU();
    void moveToFront(ListIt it);
};

// ─────────────────────────────────────────────────────────
//  Implementation (header-only for templates)
// ─────────────────────────────────────────────────────────

#include <iostream>
#include <iomanip>

template<typename K, typename V>
LRUCache<K, V>::LRUCache(int capacity, int ttl_seconds)
    : capacity_(capacity), ttlSeconds_(ttl_seconds) {
    if (capacity <= 0)
        throw std::invalid_argument("LRUCache: capacity must be > 0");
}

template<typename K, typename V>
bool LRUCache<K, V>::isExpired(const Entry& e) const {
    if (!e.hasTTL) return false;
    return Clock::now() > e.expiry;
}

template<typename K, typename V>
void LRUCache<K, V>::evictExpired() {
    // Sweep from back (LRU end) — expired entries are removed
    auto it = cacheList_.end();
    while (it != cacheList_.begin()) {
        --it;
        if (isExpired(*it)) {
            cacheMap_.erase(it->key);
            it = cacheList_.erase(it);
        }
    }
}

template<typename K, typename V>
void LRUCache<K, V>::removeLRU() {
    if (cacheList_.empty()) return;
    auto last = std::prev(cacheList_.end());
    cacheMap_.erase(last->key);
    cacheList_.erase(last);
}

template<typename K, typename V>
void LRUCache<K, V>::moveToFront(ListIt it) {
    cacheList_.splice(cacheList_.begin(), cacheList_, it);
}

// ── get ───────────────────────────────────────────────────

template<typename K, typename V>
std::optional<V> LRUCache<K, V>::get(const K& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto mapIt = cacheMap_.find(key);
    if (mapIt == cacheMap_.end()) {
        ++misses_;
        return std::nullopt;
    }

    auto listIt = mapIt->second;

    // Lazy expiry check
    if (isExpired(*listIt)) {
        cacheMap_.erase(mapIt);
        cacheList_.erase(listIt);
        ++misses_;
        return std::nullopt;
    }

    moveToFront(listIt);
    ++hits_;
    return cacheList_.front().value;
}

// ── put ───────────────────────────────────────────────────

template<typename K, typename V>
void LRUCache<K, V>::put(const K& key, const V& value) {
    std::lock_guard<std::mutex> lock(mutex_);

    // If key already exists — remove old entry
    auto mapIt = cacheMap_.find(key);
    if (mapIt != cacheMap_.end()) {
        cacheList_.erase(mapIt->second);
        cacheMap_.erase(mapIt);
    } else {
        // Evict expired entries first, then LRU if still full
        evictExpired();
        if ((int)cacheList_.size() >= capacity_) {
            removeLRU();
        }
    }

    // Build new entry
    Entry e;
    e.key    = key;
    e.value  = value;
    e.hasTTL = (ttlSeconds_ > 0);
    if (e.hasTTL)
        e.expiry = Clock::now() + std::chrono::seconds(ttlSeconds_);

    cacheList_.push_front(e);
    cacheMap_[key] = cacheList_.begin();
}

// ── erase ─────────────────────────────────────────────────

template<typename K, typename V>
bool LRUCache<K, V>::erase(const K& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto mapIt = cacheMap_.find(key);
    if (mapIt == cacheMap_.end()) return false;
    cacheList_.erase(mapIt->second);
    cacheMap_.erase(mapIt);
    return true;
}

// ── contains ──────────────────────────────────────────────

template<typename K, typename V>
bool LRUCache<K, V>::contains(const K& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto mapIt = cacheMap_.find(key);
    if (mapIt == cacheMap_.end()) return false;
    if (isExpired(*mapIt->second)) {
        cacheList_.erase(mapIt->second);
        cacheMap_.erase(mapIt);
        return false;
    }
    return true;
}

// ── size / capacity ───────────────────────────────────────

template<typename K, typename V>
int LRUCache<K, V>::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (int)cacheList_.size();
}

template<typename K, typename V>
int LRUCache<K, V>::capacity() const {
    return capacity_;
}

// ── stats ─────────────────────────────────────────────────

template<typename K, typename V>
long long LRUCache<K, V>::hits() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hits_;
}

template<typename K, typename V>
long long LRUCache<K, V>::misses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return misses_;
}

template<typename K, typename V>
double LRUCache<K, V>::hitRate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    long long total = hits_ + misses_;
    return total == 0 ? 0.0 : (double)hits_ / total * 100.0;
}

template<typename K, typename V>
void LRUCache<K, V>::resetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    hits_ = misses_ = 0;
}

// ── display ───────────────────────────────────────────────

template<typename K, typename V>
void LRUCache<K, V>::display() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "\n╔══════════════════════════════════╗\n";
    std::cout <<   "║         LRU Cache State          ║\n";
    std::cout <<   "╠══════════════════════════════════╣\n";
    std::cout <<   "║  [MRU → LRU]                     ║\n";

    int idx = 1;
    for (const auto& e : cacheList_) {
        std::cout << "║  " << idx++ << ". key=" << e.key
                  << "  val=" << e.value;
        if (e.hasTTL) {
            auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
                e.expiry - Clock::now()).count();
            std::cout << "  (TTL: " << remaining << "s)";
        }
        std::cout << "\n";
    }

    std::cout << "╠══════════════════════════════════╣\n";
    std::cout << "║  Size: " << cacheList_.size()
              << " / " << capacity_ << "\n";
    std::cout << "║  Hits: " << hits_ << "  Misses: " << misses_ << "\n";
    std::cout << "║  Hit rate: " << std::fixed << std::setprecision(1)
              << (hits_ + misses_ == 0 ? 0.0 : (double)hits_/(hits_+misses_)*100)
              << "%\n";
    std::cout << "╚══════════════════════════════════╝\n";
}
