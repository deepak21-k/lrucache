# LRU Cache

A generic, thread-safe Least Recently Used (LRU) cache in C++17 — with TTL expiry and hit/miss statistics.

---

## Features

- **Generic** — works with any key/value types via C++ templates
- **O(1) get and put** — `std::list` + `std::unordered_map` gives constant-time operations
- **Thread-safe** — `std::mutex` protects all operations for concurrent access
- **TTL expiry** — entries automatically expire after a configurable timeout
- **Hit/miss statistics** — track cache performance at runtime
- **Explicit erase** — remove entries on demand
- **22 unit tests** — covering eviction, recency, TTL, concurrency, and edge cases

---

## Data Structure Design

```
LRUCache
├── std::list<Entry>                   ← recency order (MRU at front, LRU at back)
└── std::unordered_map<K, list::iter>  ← O(1) lookup into the list
```

**Why `list + unordered_map`?**

The key insight: `std::list` iterators are stable — they never invalidate when other nodes are added or removed. So the map can store a direct iterator into the list, allowing O(1) access to any node's position without scanning.

On a `get()`:
1. Look up the iterator in the map — O(1)
2. `splice()` that node to the front of the list — O(1)
3. Return the value

On a `put()` when full:
1. Remove the back of the list (LRU entry) — O(1)
2. Erase its key from the map — O(1)
3. Push new entry to front — O(1)

| Operation | Complexity |
|-----------|------------|
| `get()`   | O(1) avg   |
| `put()`   | O(1) avg   |
| `erase()` | O(1) avg   |

---

## Build

**Requirements:** C++17, g++ or clang++, CMake ≥ 3.14 (optional)

### With CMake
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### With g++ directly
```bash
# Main binary
g++ -std=c++17 -O2 -pthread -Isrc src/main.cpp -o lru_cache

# Test binary
g++ -std=c++17 -O2 -pthread -Isrc tests/test_lrucache.cpp -o run_tests
```

---

## Usage

```cpp
#include "LRUCache.h"

// Basic usage — capacity 3, no TTL
LRUCache<int, std::string> cache(3);

cache.put(1, "Alice");
cache.put(2, "Bob");
cache.put(3, "Charlie");

auto val = cache.get(1);   // returns std::optional<std::string>
if (val) std::cout << *val;

// With TTL — entries expire after 60 seconds
LRUCache<std::string, std::string> ttlCache(100, 60);
ttlCache.put("session:abc", "user_data");

// Statistics
std::cout << cache.hitRate() << "%\n";
```

---

## Running Tests

```bash
./run_tests
```

Expected:
```
Results: 22 passed, 0 failed
```

Tests cover: basic eviction, recency updates, key updates, explicit erase, hit/miss stats, template types, TTL expiry, thread safety, invalid capacity.

---

## Project Structure

```
lru_cache/
├── src/
│   ├── LRUCache.h      # Full implementation (header-only, templated)
│   └── main.cpp        # Demo — all features shown
├── tests/
│   └── test_lrucache.cpp
├── CMakeLists.txt
└── README.md
```

---

## Possible Extensions

- **LFU eviction** — evict least *frequently* used instead of least recently used
- **Max memory limit** — evict based on byte size rather than entry count
- **Async TTL cleanup** — background thread to sweep expired entries
- **Persistence** — serialize cache state to disk and reload on startup
