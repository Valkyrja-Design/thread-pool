#include <algorithm>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <thread_pool.h>
#include <vector>

// Performance test configuration
struct TestConfig {
    std::size_t num_threads = std::thread::hardware_concurrency();
    std::size_t num_tasks = 10000;
    std::size_t num_iterations = 5;
    bool verbose = false;
};

// Simple CPU-intensive task
std::size_t fibonacci(std::size_t n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

// I/O simulation task
void simulate_io_work(std::chrono::microseconds duration) {
    std::this_thread::sleep_for(duration);
}

// Memory-intensive task
std::size_t memory_work(std::size_t size) {
    std::vector<int> data(size);
    std::iota(data.begin(), data.end(), 0);
    return std::accumulate(data.begin(), data.end(), 0ULL);
}

// Benchmark helper
template <typename Func>
double measure_execution_time(Func&& func,
                              const std::string& test_name,
                              bool verbose = false) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration<double, std::milli>(end - start).count();

    if (verbose) {
        std::cout << test_name << ": " << std::fixed << std::setprecision(2)
                  << duration << " ms" << std::endl;
    }

    return duration;
}

// Test 1: Task submission overhead
void test_submission_overhead(const TestConfig& config) {
    std::cout << "\n=== Task Submission Overhead Test ===" << std::endl;
    std::cout << "Tasks: " << config.num_tasks
              << ", Threads: " << config.num_threads << std::endl;

    std::vector<double> times;

    for (std::size_t iter = 0; iter < config.num_iterations; ++iter) {
        ThreadPool pool(config.num_threads);
        std::vector<std::future<int>> futures;
        futures.reserve(config.num_tasks);

        // Measure ONLY the submission time
        auto time = measure_execution_time(
            [&]() {
                for (std::size_t i = 0; i < config.num_tasks; ++i) {
                    futures.emplace_back(pool.submit_task(
                        [i]() { return static_cast<int>(i); }));
                }
            },
            "Submission overhead iteration " + std::to_string(iter),
            config.verbose);

        times.push_back(time);

        for (auto& future : futures) {
            future.get();
        }
    }

    double avg_time = std::accumulate(times.begin(), times.end(), 0.0) /
                      times.size();
    double submissions_per_second = (config.num_tasks * 1000.0) / avg_time;
    double microseconds_per_task = (avg_time * 1000.0) / config.num_tasks;

    std::cout << "Average submission time: " << std::fixed
              << std::setprecision(2) << avg_time << " ms" << std::endl;
    std::cout << "Submissions/second: " << std::fixed << std::setprecision(0)
              << submissions_per_second << std::endl;
    std::cout << "Microseconds per submission: " << std::fixed
              << std::setprecision(2) << microseconds_per_task << " μs"
              << std::endl;
}

// Test 1.5: End-to-end task throughput
void test_end_to_end_throughput(const TestConfig& config) {
    std::cout << "\n=== End-to-End Task Throughput Test ===" << std::endl;
    std::cout << "Tasks: " << config.num_tasks
              << ", Threads: " << config.num_threads << std::endl;

    std::vector<double> times;

    for (std::size_t iter = 0; iter < config.num_iterations; ++iter) {
        ThreadPool pool(config.num_threads);
        std::vector<std::future<int>> futures;
        futures.reserve(config.num_tasks);

        auto time = measure_execution_time(
            [&]() {
                for (std::size_t i = 0; i < config.num_tasks; ++i) {
                    futures.emplace_back(pool.submit_task(
                        [i]() { return static_cast<int>(i); }));
                }

                for (auto& future : futures) {
                    future.get();
                }
            },
            "End-to-end iteration " + std::to_string(iter),
            config.verbose);

        times.push_back(time);
    }

    double avg_time = std::accumulate(times.begin(), times.end(), 0.0) /
                      times.size();
    double tasks_per_second = (config.num_tasks * 1000.0) / avg_time;

    std::cout << "Average end-to-end time: " << std::fixed
              << std::setprecision(2) << avg_time << " ms" << std::endl;
    std::cout << "End-to-end tasks/second: " << std::fixed
              << std::setprecision(0) << tasks_per_second << std::endl;
}

// Test 2: CPU-intensive workload
void test_cpu_intensive(const TestConfig& config) {
    std::cout << "\n=== CPU-Intensive Workload Test ===" << std::endl;

    constexpr std::size_t fib_number = 35;
    const std::size_t cpu_tasks = config.num_threads *
                                  20;  // Scale with thread count

    std::vector<double> times;

    for (std::size_t iter = 0; iter < config.num_iterations; ++iter) {
        ThreadPool pool(config.num_threads);
        std::vector<std::future<std::size_t>> futures;
        futures.reserve(cpu_tasks);

        auto time = measure_execution_time(
            [&]() {
                for (std::size_t i = 0; i < cpu_tasks; ++i) {
                    futures.emplace_back(pool.submit_task(
                        []() { return fibonacci(fib_number); }));
                }

                std::size_t total = 0;
                for (auto& future : futures) {
                    total += future.get();
                }

                if (config.verbose) {
                    std::cout << "Total fibonacci results: " << total
                              << std::endl;
                }
            },
            "CPU-intensive iteration " + std::to_string(iter),
            config.verbose);

        times.push_back(time);
    }

    double avg_time = std::accumulate(times.begin(), times.end(), 0.0) /
                      times.size();
    std::cout << "Average time: " << std::fixed << std::setprecision(2)
              << avg_time << " ms" << std::endl;
    std::cout << "Tasks: " << cpu_tasks << ", Throughput: " << std::fixed
              << std::setprecision(1) << (cpu_tasks * 1000.0) / avg_time
              << " tasks/sec" << std::endl;
}

// Test 3: Mixed workload with different task types
void test_mixed_workload(const TestConfig& config) {
    std::cout << "\n=== Mixed Workload Test ===" << std::endl;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> task_type_dist(0, 2);
    std::uniform_int_distribution<> io_duration_dist(1, 10);  // 1-10 ms
    std::uniform_int_distribution<> memory_size_dist(1000, 10000);

    std::vector<double> times;

    for (std::size_t iter = 0; iter < config.num_iterations; ++iter) {
        ThreadPool pool(config.num_threads);
        std::vector<std::future<void>> futures;
        futures.reserve(config.num_tasks);

        auto time = measure_execution_time(
            [&]() {
                for (std::size_t i = 0; i < config.num_tasks; ++i) {
                    int task_type = task_type_dist(gen);

                    switch (task_type) {
                        case 0:  // CPU task
                            futures.emplace_back(pool.submit_task([]() {
                                fibonacci(25);  // Lighter CPU work
                            }));
                            break;

                        case 1:  // I/O simulation
                            futures.emplace_back(pool.submit_task(
                                [&gen, &io_duration_dist]() -> void {
                                    auto duration = std::chrono::microseconds(
                                        io_duration_dist(gen) * 1000);
                                    simulate_io_work(duration);
                                }));
                            break;

                        case 2:  // Memory work
                            futures.emplace_back(pool.submit_task(
                                [&gen, &memory_size_dist]() -> void {
                                    memory_work(memory_size_dist(gen));
                                }));
                            break;
                    }
                }

                for (auto& future : futures) {
                    future.get();
                }
            },
            "Mixed workload iteration " + std::to_string(iter),
            config.verbose);

        times.push_back(time);
    }

    double avg_time = std::accumulate(times.begin(), times.end(), 0.0) /
                      times.size();
    std::cout << "Average time: " << std::fixed << std::setprecision(2)
              << avg_time << " ms" << std::endl;
    std::cout << "Mixed tasks/second: " << std::fixed << std::setprecision(0)
              << (config.num_tasks * 1000.0) / avg_time << std::endl;
}

// Test 4: Exception handling performance
void test_exception_handling(const TestConfig& config) {
    std::cout << "\n=== Exception Handling Performance Test ===" << std::endl;

    const std::size_t exception_tasks = 1000;
    std::vector<double> times;

    for (std::size_t iter = 0; iter < config.num_iterations; ++iter) {
        ThreadPool pool(config.num_threads);
        std::vector<std::future<int>> futures;
        futures.reserve(exception_tasks);

        auto time = measure_execution_time(
            [&]() {
                // Submit tasks that throw exceptions
                for (std::size_t i = 0; i < exception_tasks; ++i) {
                    futures.emplace_back(pool.submit_task([i]() -> int {
                        if (i % 2 == 0) {
                            throw std::runtime_error("Test exception " +
                                                     std::to_string(i));
                        }
                        return static_cast<int>(i);
                    }));
                }

                // Handle results and exceptions
                std::size_t exception_count = 0;
                for (auto& future : futures) {
                    try {
                        future.get();
                    } catch (const std::exception&) {
                        ++exception_count;
                    }
                }

                if (exception_count != futures.size() / 2) {
                    throw std::runtime_error("Unexpected number of exceptions");
                }

                if (config.verbose) {
                    std::cout << "Caught " << exception_count << " exceptions"
                              << std::endl;
                }
            },
            "Exception handling iteration " + std::to_string(iter),
            config.verbose);

        times.push_back(time);
    }

    double avg_time = std::accumulate(times.begin(), times.end(), 0.0) /
                      times.size();
    std::cout << "Average time: " << std::fixed << std::setprecision(2)
              << avg_time << " ms" << std::endl;
    std::cout << "Exception tasks/second: " << std::fixed
              << std::setprecision(0) << (exception_tasks * 1000.0) / avg_time
              << std::endl;
}

// Test 5: Scalability test
void test_scalability(const TestConfig&) {
    std::cout << "\n=== Thread Scalability Test ===" << std::endl;

    const std::size_t base_tasks = 5000;
    std::vector<std::size_t> thread_counts = {1, 2, 4, 8, 16, 32};

    // Filter thread counts to reasonable values
    std::size_t max_threads = std::thread::hardware_concurrency() * 2;
    thread_counts.erase(std::remove_if(thread_counts.begin(),
                                       thread_counts.end(),
                                       [max_threads](std::size_t n) {
                                           return n > max_threads;
                                       }),
                        thread_counts.end());

    std::cout << "Base tasks per test: " << base_tasks << std::endl;
    std::cout << std::setw(8) << "Threads" << std::setw(12) << "Time (ms)"
              << std::setw(15) << "Tasks/sec" << std::setw(12) << "Efficiency"
              << std::endl;
    std::cout << std::string(50, '-') << std::endl;

    double baseline_time = 0;

    for (std::size_t num_threads : thread_counts) {
        std::vector<double> times;

        for (std::size_t iter = 0; iter < 3;
             ++iter) {  // Fewer iterations for scalability test
            ThreadPool pool(num_threads);
            std::vector<std::future<int>> futures;
            futures.reserve(base_tasks);

            auto time = measure_execution_time(
                [&]() {
                    for (std::size_t i = 0; i < base_tasks; ++i) {
                        futures.emplace_back(pool.submit_task([i]() {
                            fibonacci(30);  // Moderate CPU work
                            return static_cast<int>(i);
                        }));
                    }

                    for (auto& future : futures) {
                        future.get();
                    }
                },
                "Scalability test",
                false);

            times.push_back(time);
        }

        double avg_time = std::accumulate(times.begin(), times.end(), 0.0) /
                          times.size();
        double tasks_per_sec = (base_tasks * 1000.0) / avg_time;

        if (num_threads == 1) {
            baseline_time = avg_time;
        }

        double efficiency = (baseline_time / avg_time) / num_threads * 100.0;

        std::cout << std::setw(8) << num_threads << std::setw(12) << std::fixed
                  << std::setprecision(1) << avg_time << std::setw(15)
                  << std::fixed << std::setprecision(0) << tasks_per_sec
                  << std::setw(11) << std::fixed << std::setprecision(1)
                  << efficiency << "%" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    TestConfig config;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            config.verbose = true;
        } else if (arg == "--tasks" && i + 1 < argc) {
            config.num_tasks = std::stoull(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            config.num_threads = std::stoull(argv[++i]);
        } else if (arg == "--iterations" && i + 1 < argc) {
            config.num_iterations = std::stoull(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout
                << "Usage: " << argv[0] << " [options]\n"
                << "Options:\n"
                << "  --tasks N       Number of tasks per test (default: "
                << config.num_tasks << ")\n"
                << "  --threads N     Number of threads (default: "
                << config.num_threads << ")\n"
                << "  --iterations N  Number of test iterations (default: "
                << config.num_iterations << ")\n"
                << "  --verbose, -v   Enable verbose output\n"
                << "  --help, -h      Show this help\n";
            return 0;
        }
    }

    std::cout << "ThreadPool Performance Tests" << std::endl;
    std::cout << "=============================" << std::endl;
    std::cout << "Hardware threads: " << std::thread::hardware_concurrency()
              << std::endl;
    std::cout << "Test configuration:" << std::endl;
    std::cout << "  Threads: " << config.num_threads << std::endl;
    std::cout << "  Tasks: " << config.num_tasks << std::endl;
    std::cout << "  Iterations: " << config.num_iterations << std::endl;
    std::cout << "  Verbose: " << (config.verbose ? "Yes" : "No") << std::endl;

    try {
        test_submission_overhead(config);
        test_end_to_end_throughput(config);
        test_cpu_intensive(config);
        test_mixed_workload(config);
        test_exception_handling(config);
        test_scalability(config);

        std::cout << "\n=== Performance Tests Completed ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
