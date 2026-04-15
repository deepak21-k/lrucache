cmake_minimum_required(VERSION 3.14)
project(lru_cache CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Threads required for mutex / thread safety
find_package(Threads REQUIRED)

# ── Main executable ────────────────────────────────────────
add_executable(lru_cache src/main.cpp)
target_include_directories(lru_cache PRIVATE src)
target_link_libraries(lru_cache Threads::Threads)

# ── Tests ──────────────────────────────────────────────────
enable_testing()

add_executable(run_tests tests/test_lrucache.cpp)
target_include_directories(run_tests PRIVATE src)
target_link_libraries(run_tests Threads::Threads)

add_test(NAME LRUCacheTests COMMAND run_tests)
