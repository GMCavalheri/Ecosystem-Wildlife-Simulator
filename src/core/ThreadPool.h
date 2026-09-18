#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace eco {

// Phase 7: a minimal persistent fork-join thread pool. parallelFor(count, body) runs
// body(0) ... body(count-1) across the pool and blocks until every call has returned;
// the calling thread participates, so a pool of N threads spawns N-1 workers. Workers
// sleep between jobs (no spinning), and work is handed out one index at a time from a
// shared atomic counter, so uneven chunks balance themselves.
//
// The body must not throw, and calls from different indices must not race with each
// other -- the pool provides the fork/join barrier, not any synchronization inside body.
class ThreadPool {
public:
    // totalThreads >= 1; 1 means "no workers, everything runs inline on the caller".
    explicit ThreadPool(unsigned totalThreads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    unsigned threadCount() const { return static_cast<unsigned>(workers_.size()) + 1; }

    void parallelFor(std::size_t count, const std::function<void(std::size_t)>& body);

private:
    void workerLoop();
    void runChunks();

    std::vector<std::thread> workers_;
    std::mutex mutex_;
    std::condition_variable workReady_;
    std::condition_variable workDone_;
    std::uint64_t generation_ = 0;
    bool stopping_ = false;

    // Current job -- written by parallelFor under mutex_ before workers are woken.
    const std::function<void(std::size_t)>* body_ = nullptr;
    std::size_t count_ = 0;
    std::atomic<std::size_t> next_{0};
    std::size_t workersPending_ = 0;
};

} // namespace eco
