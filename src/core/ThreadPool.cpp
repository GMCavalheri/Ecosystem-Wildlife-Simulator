#include "core/ThreadPool.h"

namespace eco {

ThreadPool::ThreadPool(unsigned totalThreads) {
    const unsigned workers = totalThreads > 1 ? totalThreads - 1 : 0;
    workers_.reserve(workers);
    for (unsigned i = 0; i < workers; ++i) {
        workers_.emplace_back([this] { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
    }
    workReady_.notify_all();
    for (auto& worker : workers_) {
        worker.join();
    }
}

void ThreadPool::runChunks() {
    for (;;) {
        const std::size_t index = next_.fetch_add(1, std::memory_order_relaxed);
        if (index >= count_) {
            return;
        }
        (*body_)(index);
    }
}

void ThreadPool::workerLoop() {
    std::uint64_t seen = 0;
    for (;;) {
        {
            std::unique_lock lock(mutex_);
            workReady_.wait(lock, [&] { return stopping_ || generation_ != seen; });
            if (stopping_) {
                return;
            }
            seen = generation_;
        }

        runChunks();

        // Every worker checks in exactly once per job, even if it found no work left,
        // so parallelFor can't return (and invalidate body_) while any worker is still
        // on its way in.
        std::lock_guard lock(mutex_);
        if (--workersPending_ == 0) {
            workDone_.notify_one();
        }
    }
}

void ThreadPool::parallelFor(std::size_t count, const std::function<void(std::size_t)>& body) {
    if (workers_.empty() || count <= 1) {
        for (std::size_t i = 0; i < count; ++i) {
            body(i);
        }
        return;
    }

    {
        std::lock_guard lock(mutex_);
        body_ = &body;
        count_ = count;
        next_.store(0, std::memory_order_relaxed);
        workersPending_ = workers_.size();
        ++generation_;
    }
    workReady_.notify_all();

    runChunks();

    std::unique_lock lock(mutex_);
    workDone_.wait(lock, [&] { return workersPending_ == 0; });
    body_ = nullptr;
}

} // namespace eco
