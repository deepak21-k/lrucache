#include "LRUCache.h"

#include <iostream>
#include <string>
#include <thread>
#include <vector>

// ── Demo 1: Basic int→string cache (your original) ───────
void demo_basic() {
    std::cout << "\n══ Demo 1: Basic LRU (int → string) ══\n";

    LRUCache<int, std::string> cache(3);

    cache.put(1, "A");
    cache.put(2, "B");
    cache.put(3, "C");
    cache.display();

    cache.get(1);           // 1 becomes MRU
    cache.put(4, "D");      // evicts LRU (key=2)
    cache.display();

    auto v = cache.get(2);
    std::cout << "Get key=2: " << (v ? *v : "NOT FOUND") << "\n";

    auto v2 = cache.get(1);
    std::cout << "Get key=1: " << (v2 ? *v2 : "NOT FOUND") << "\n";
}

// ── Demo 2: Templates — string→string cache ───────────────
void demo_templates() {
    std::cout << "\n══ Demo 2: Templated (string → string) ══\n";

    LRUCache<std::string, std::string> cache(3);

    cache.put("user:1", "Alice");
    cache.put("user:2", "Bob");
    cache.put("user:3", "Charlie");

    cache.get("user:1");           // user:1 → MRU
    cache.put("user:4", "Diana");  // evicts user:2

    cache.display();

    std::cout << "Contains user:2? " << (cache.contains("user:2") ? "yes" : "no") << "\n";
    std::cout << "Contains user:1? " << (cache.contains("user:1") ? "yes" : "no") << "\n";
}

// ── Demo 3: Hit/miss statistics ───────────────────────────
void demo_stats() {
    std::cout << "\n══ Demo 3: Hit/Miss Statistics ══\n";

    LRUCache<int, std::string> cache(3);

    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");

    cache.get(1);   // hit
    cache.get(2);   // hit
    cache.get(99);  // miss
    cache.get(100); // miss
    cache.get(3);   // hit

    std::cout << "Hits:     " << cache.hits()   << "\n";
    std::cout << "Misses:   " << cache.misses() << "\n";
    std::cout << "Hit rate: " << cache.hitRate() << "%\n";
}

// ── Demo 4: TTL expiry ────────────────────────────────────
void demo_ttl() {
    std::cout << "\n══ Demo 4: TTL Expiry (2 second TTL) ══\n";

    LRUCache<int, std::string> cache(5, 2); // 2 second TTL

    cache.put(1, "expires_soon");
    cache.put(2, "also_expires");

    auto v1 = cache.get(1);
    std::cout << "Before expiry — key=1: " << (v1 ? *v1 : "NOT FOUND") << "\n";

    std::cout << "Waiting 3 seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    auto v2 = cache.get(1);
    std::cout << "After expiry  — key=1: " << (v2 ? *v2 : "NOT FOUND") << "\n";
}

// ── Demo 5: Thread safety ─────────────────────────────────
void demo_threads() {
    std::cout << "\n══ Demo 5: Thread Safety ══\n";

    LRUCache<int, std::string> cache(100);

    // 4 threads writing concurrently
    std::vector<std::thread> writers;
    for (int t = 0; t < 4; ++t) {
        writers.emplace_back([&cache, t]() {
            for (int i = 0; i < 25; ++i) {
                cache.put(t * 25 + i, "val_" + std::to_string(t * 25 + i));
            }
        });
    }
    for (auto& w : writers) w.join();

    // 4 threads reading concurrently
    std::vector<std::thread> readers;
    for (int t = 0; t < 4; ++t) {
        readers.emplace_back([&cache, t]() {
            for (int i = 0; i < 25; ++i) {
                cache.get(t * 25 + i);
            }
        });
    }
    for (auto& r : readers) r.join();

    std::cout << "100 concurrent writes + 100 concurrent reads — no crash\n";
    std::cout << "Final size: " << cache.size() << "\n";
    std::cout << "Hit rate:   " << cache.hitRate() << "%\n";
}

int main() {
    demo_basic();
    demo_templates();
    demo_stats();
    demo_ttl();
    demo_threads();

    std::cout << "\n[INFO] All demos complete.\n";
    return 0;
    
}
