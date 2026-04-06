#include "core/job_system.h"
#include <algorithm>

namespace kairo {

void JobSystem::init(u32 num_threads) {
    if (m_running) return;

    if (num_threads == 0) {
        u32 hw = std::thread::hardware_concurrency();
        num_threads = (hw > 1) ? (hw - 1) : 1;
    }

    m_running = true;

    m_workers.reserve(num_threads);
    for (u32 i = 0; i < num_threads; ++i) {
        m_workers.emplace_back([this] {
            while (true) {
                std::function<void()> job;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_condition.wait(lock, [this] {
                        return !m_running || !m_jobs.empty();
                    });

                    if (!m_running && m_jobs.empty()) return;

                    job = std::move(m_jobs.front());
                    m_jobs.pop();
                }

                job();

                // decrement pending count and notify idle waiters
                if (--m_pending == 0) {
                    m_idle_condition.notify_all();
                }
            }
        });
    }
}

void JobSystem::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
    }
    m_condition.notify_all();

    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_workers.clear();
}

std::future<void> JobSystem::submit(std::function<void()> job) {
    auto task = std::make_shared<std::packaged_task<void()>>(std::move(job));
    auto future = task->get_future();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_pending;
        m_jobs.emplace([task]() { (*task)(); });
    }
    m_condition.notify_one();

    return future;
}

void JobSystem::parallel_for(u32 count, const std::function<void(u32 index)>& func) {
    if (count == 0) return;

    u32 threads = static_cast<u32>(m_workers.size());
    if (threads == 0) {
        // fallback: run on calling thread
        for (u32 i = 0; i < count; ++i) func(i);
        return;
    }

    u32 chunk_size = (count + threads - 1) / threads;
    std::vector<std::future<void>> futures;
    futures.reserve(threads);

    for (u32 t = 0; t < threads; ++t) {
        u32 begin = t * chunk_size;
        u32 end = std::min(begin + chunk_size, count);
        if (begin >= count) break;

        futures.push_back(submit([&func, begin, end]() {
            for (u32 i = begin; i < end; ++i) {
                func(i);
            }
        }));
    }

    for (auto& f : futures) {
        f.wait();
    }
}

void JobSystem::wait_idle() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_idle_condition.wait(lock, [this] {
        return m_pending == 0;
    });
}

u32 JobSystem::thread_count() const {
    return static_cast<u32>(m_workers.size());
}

u32 JobSystem::pending_jobs() const {
    return m_pending.load();
}

} // namespace kairo
