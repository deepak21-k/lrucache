#include "LRUCache.h"

#include <iostream>
#include <thread>
#include <vector>
#include <string>

static int passed = 0;
static int failed = 0;

#define EXPECT(cond, msg)                                           \
    do {                                                            \
        if (cond) {                                                 \
            std::cout << "  [PASS] " << msg << "\n"; ++passed;     \
        } else {                                                    \
            std::cout << "  [FAIL] " << msg << "\n"; ++failed;     \
        }                                                           \
    } while(0)

// ── Basic eviction ────────────────────────────────────────
void test_basic_eviction() {
    std::cout << "\n[TEST] Basic LRU eviction\n";
    LRUCache<int, std::string> c(3);

    c.put(1, "a"); c.put(2, "b"); c.put(3, "c");
    c.put(4, "d"); // evicts key=1

    EXPECT(!c.get(1).has_value(),  "key=1 evicted");
    EXPECT(c.get(2).has_value(),   "key=2 still present");
    EXPECT(c.get(3).has_value(),   "key=3 still present");
    EXPECT(c.get(4).has_value(),   "key=4 inserted");
}

// ── Recency update ────────────────────────────────────────
void test_recency() {
    std::cout << "\n[TEST] Recency update on get\n";
    LRUCache<int, std::string> c(3);

    c.put(1, "a"); c.put(2, "b"); c.put(3, "c");
    c.get(1);      // 1 is now MRU
    c.put(4, "d"); // should evict 2, not 1

    EXPECT(c.get(1).has_value(),  "key=1 not evicted (was recently used)");
    EXPECT(!c.get(2).has_value(), "key=2 evicted (was LRU)");
}

// ── Update existing key ───────────────────────────────────
void test_update() {
    std::cout << "\n[TEST] Update existing key\n";
    LRUCache<int, std::string> c(3);

    c.put(1, "old");
    c.put(1, "new");

    EXPECT(c.size() == 1,              "Size stays 1 after update");
    EXPECT(*c.get(1) == "new",         "Value updated correctly");
}

// ── Erase ─────────────────────────────────────────────────
void test_erase() {
    std::cout << "\n[TEST] Explicit erase\n";
    LRUCache<int, std::string> c(3);

    c.put(1, "a");
    bool ok  = c.erase(1);
    bool bad = c.erase(99);

    EXPECT(ok,                    "Erase existing key returns true");
    EXPECT(!bad,                  "Erase missing key returns false");
    EXPECT(!c.get(1).has_value(), "Erased key not found");
}

// ── Stats ─────────────────────────────────────────────────
void test_stats() {
    std::cout << "\n[TEST] Hit/miss statistics\n";
    LRUCache<int, std::string> c(3);

    c.put(1, "a");
    c.get(1);   // hit
    c.get(2);   // miss

    EXPECT(c.hits()   == 1,    "1 hit");
    EXPECT(c.misses() == 1,    "1 miss");
    EXPECT(c.hitRate() == 50.0,"Hit rate 50%");

    c.resetStats();
    EXPECT(c.hits() == 0,      "Stats reset");
}

// ── Templates ─────────────────────────────────────────────
void test_templates() {
    std::cout << "\n[TEST] Template types (string → string)\n";
    LRUCache<std::string, std::string> c(2);

    c.put("a", "alpha");
    c.put("b", "beta");
    c.put("c", "gamma"); // evicts "a"

    EXPECT(!c.get("a").has_value(),     "key=a evicted");
    EXPECT(*c.get("b") == "beta",       "key=b correct");
    EXPECT(*c.get("c") == "gamma",      "key=c correct");
}

// ── TTL expiry ────────────────────────────────────────────
void test_ttl() {
    std::cout << "\n[TEST] TTL expiry\n";
    LRUCache<int, std::string> c(5, 1); // 1 second TTL

    c.put(1, "expires");
    EXPECT(c.get(1).has_value(), "Key present before expiry");

    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    EXPECT(!c.get(1).has_value(), "Key gone after TTL");
}

// ── Thread safety ─────────────────────────────────────────
void test_thread_safety() {
    std::cout << "\n[TEST] Thread safety\n";
    LRUCache<int, std::string> c(50);

    std::vector<std::thread> threads;
    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&c, t]() {
            for (int i = 0; i < 100; ++i) {
                c.put(t * 100 + i, "v");
                c.get(t * 100 + i);
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT(c.size() <= 50, "Size within capacity after concurrent ops");
}

// ── Capacity guard ────────────────────────────────────────
void test_invalid_capacity() {
    std::cout << "\n[TEST] Invalid capacity throws\n";
    bool threw = false;
    try {
        LRUCache<int, int> c(0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    EXPECT(threw, "Capacity=0 throws invalid_argument");
}

int main() {
    std::cout << "═══════════════════════════════════════\n";
    std::cout << "  LRU Cache — Test Suite\n";
    std::cout << "═══════════════════════════════════════\n";

    test_basic_eviction();
    test_recency();
    test_update();
    test_erase();
    test_stats();
    test_templates();
    test_ttl();
    test_thread_safety();
    test_invalid_capacity();

    std::cout << "\n───────────────────────────────────────\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    std::cout << "───────────────────────────────────────\n";

    return failed == 0 ? 0 : 1;
}
