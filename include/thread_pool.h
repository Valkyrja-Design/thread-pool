#include <chrono>
#include <concepts>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

template <bool EnablePriorityScheduling = true>
class ThreadPool {
  public:
    using Priority = std::int8_t;

  private:
    class WorkQueue {
        struct WorkItem {
            std::function<void()> task;
            Priority priority;

            bool operator<(const WorkItem& other) const {
                return priority < other.priority;
            }
        };

      public:
        void push(WorkItem work_item) {
            work_queue.push(std::move(work_item));
        }

        WorkItem pop() {
            WorkItem work_item;

            if constexpr (EnablePriorityScheduling) {
                work_item = work_queue.top();
            } else {
                work_item = work_queue.front();
            }

            work_queue.pop();
            return work_item;
        }

        bool empty() const {
            return work_queue.empty();
        }

      private:
        std::conditional_t<EnablePriorityScheduling,
                           std::priority_queue<WorkItem>,
                           std::queue<WorkItem>>
            work_queue;
    };

  public:
    explicit ThreadPool(std::size_t num_threads)
        : num_threads{num_threads}, tasks_running{num_threads} {
        threads.reserve(num_threads);

        for (std::size_t i = 0; i < num_threads; ++i) {
            threads.emplace_back([this]() mutable {
                while (true) {
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        --tasks_running;

                        cv.wait(lock, [this]() {
                            return stop || !work_queue.empty();
                        });

                        if (stop) {
                            return;
                        }

                        auto work_item = work_queue.pop();

                        ++tasks_running;
                        lock.unlock();
                        work_item.task();
                    }
                }
            });
        }
    }

    template <typename F, typename... Args>
        requires std::invocable<F, Args...>
    std::future<std::invoke_result_t<F, Args...>> submit_task(F&& f,
                                                              Args&&... args) {
        return submit_task_helper(
            0, std::forward<F>(f), std::forward<Args>(args)...);
    }

    template <typename F, typename... Args>
        requires std::invocable<F, Args...>
    std::future<std::invoke_result_t<F, Args...>> submit_priority_task(
        Priority priority, F&& f, Args&&... args) {
        static_assert(EnablePriorityScheduling,
                      "Priority scheduling should be enabled");
        return submit_task_helper(
            priority, std::forward<F>(f), std::forward<Args>(args)...);
    }

    std::size_t thread_count() const {
        return num_threads;
    }

    std::size_t get_tasks_running() const {
        std::lock_guard<std::mutex> lock(queue_mutex);
        return tasks_running;
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            stop = true;
        }
        cv.notify_all();

        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

  private:
    template <typename F, typename... Args>
        requires std::invocable<F, Args...>
    std::future<std::invoke_result_t<F, Args...>> submit_task_helper(
        Priority priority, F&& f, Args&&... args) {
        using ResultType = std::invoke_result_t<F, Args...>;

        auto promise = std::make_shared<std::promise<ResultType>>();
        std::future<ResultType> result = promise->get_future();

        std::function<void()> work_item =
            [f = std::forward<F>(f),
             ... args = std::forward<Args>(args),
             promise = std::move(promise)]() mutable {
                try {
                    if constexpr (std::is_void_v<ResultType>) {
                        std::invoke(f, args...);
                        promise->set_value();
                    } else {
                        promise->set_value(std::invoke(f, args...));
                    }
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            };

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            work_queue.push({work_item, priority});
        }

        cv.notify_one();
        return result;
    }

  private:
    std::size_t num_threads;
    std::vector<std::thread> threads;
    WorkQueue work_queue;
    mutable std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop = false;
    std::size_t tasks_running;
};
