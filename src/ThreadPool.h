#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <memory>

/**
 * Thread Pool personnalisé pour gérer la concurrence
 * Élimine le modèle thread-per-request pour supporter C10k
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    ~ThreadPool();

    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // Ajouter une tâche à la queue
    template<typename F, typename... Args>
    void enqueue(F&& f, Args&&... args);

    // Arrêter le thread pool
    void shutdown();

    // Nombre de threads actifs
    size_t size() const { return threads_.size(); }

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};

    void worker_thread();
};

template<typename F, typename... Args>
void ThreadPool::enqueue(F&& f, Args&&... args) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) {
            return;
        }
        tasks_.emplace([f = std::forward<F>(f), args...]() mutable {
            std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        });
    }
    condition_.notify_one();
}
