#pragma once
#include "core/types.h"
#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>

namespace kairo {

class JobSystem {
public:
    // initialize with N worker threads (0 = hardware_concurrency - 1)
    void init(u32 num_threads = 0);
    void shutdown();

    // submit a job, returns a future to wait on
    std::future<void> submit(std::function<void()> job);

    // parallel for: splits range [0, count) across threads
    void parallel_for(u32 count, const std::function<void(u32 index)>& func);

    // wait for all submitted jobs to complete
    void wait_idle();

    u32 thread_count() const;
    u32 pending_jobs() const;

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_jobs;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_running{false};
    std::atomic<u32> m_pending{0};
    std::condition_variable m_idle_condition;
};

} // namespace kairo
