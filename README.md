# Introduction

A simple threadpool implementation using C++ futures and promises

## Examples

```cpp
#include "thread_pool.h"
#include <iostream>
#include <chrono>

int main() {
    constexpr std::size_t num_threads = 4;
    ThreadPool pool(num_threads);

    auto future = pool.submit_task([](int a, int b) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return a + b;
    }, 5, 10);

    std::cout << "Result: " << future.get() << std::endl;

    // Priority tasks
    ThreadPool<true> priority_pool(num_threads);
    std::int8_t priority = 16;

    auto priority_future = priority_pool.submit_priority_task(priority, [](int a, int b) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return a + b;
    }, 5, 10);

    std::cout << "Priority Result: " << priority_future.get() << std::endl;

    return 0;
}
```
