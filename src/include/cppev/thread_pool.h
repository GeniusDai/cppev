#ifndef _thread_pool_h_6C0224787A17_
#define _thread_pool_h_6C0224787A17_

#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <type_traits>
#include <vector>
#include <functional>
#include "cppev/utils.h"
#include "cppev/runnable.h"

namespace cppev
{

template<typename Runnable, typename... Args>
class thread_pool
{
    static_assert(std::is_base_of<runnable, Runnable>::value, "Not runnable");
    static_assert(std::is_constructible<Runnable, Args&&...>::value, "Not constructible");
public:
    explicit thread_pool(int thr_num, Args&&... args)
    {
        for (int i = 0; i < thr_num; ++i)
        {
            thrs_.push_back(std::make_unique<Runnable>(std::forward<Args>(args)...));
        }
    }

    thread_pool(const thread_pool &) = delete;
    thread_pool &operator=(const thread_pool &) = delete;

    thread_pool(thread_pool &&other) = default;
    thread_pool &operator=(thread_pool &&other) = default;

    virtual ~thread_pool() = default;

    // Run all threads
    void run()
    {
        for (auto &thr : thrs_)
        {
            thr->run();
        }
    }

    // Wait for all threads
    void join()
    {
        for (auto &thr : thrs_)
        {
            thr->join();
        }
    }

    // Cancel all threads
    virtual void cancel()
    {
        for (auto &thr : thrs_)
        {
            thr->cancel();
        }
    }

    // Specific const one of the threads
    const Runnable &operator[](int i) const noexcept
    {
        return *(thrs_[i].get());
    }

    // Specific one of the threads
    Runnable &operator[](int i) noexcept
    {
        return *(thrs_[i].get());
    }

    // Thread pool size
    int size() const noexcept
    {
        return thrs_.size();
    }

protected:
    std::vector<std::unique_ptr<Runnable>> thrs_;
};

namespace task_queue
{

using thread_pool_task_handler = std::function<void(void)>;

class task_queue
{
    friend class thread_pool_task_queue_runnable;
public:
    task_queue() noexcept
    : stop_(false)
    {
    }

    virtual ~task_queue() = default;

    void add_task(const thread_pool_task_handler &h) noexcept
    {
        std::unique_lock<std::mutex> lock(lock_);
        queue_.push(h);
        cond_.notify_one();
    }

    void add_task(thread_pool_task_handler &&h) noexcept
    {
        std::unique_lock<std::mutex> lock(lock_);
        queue_.push(std::forward<thread_pool_task_handler>(h));
        cond_.notify_one();
    }

    void add_task(const std::vector<thread_pool_task_handler> &vh) noexcept
    {
        std::unique_lock<std::mutex> lock(lock_);
        for (const auto &h : vh)
        {
            queue_.push(h);
        }
        cond_.notify_all();
    }

protected:
    std::queue<thread_pool_task_handler> queue_;

    std::mutex lock_;

    std::condition_variable cond_;

    bool stop_;
};

class thread_pool_task_queue_runnable final
: public runnable
{
public:
    thread_pool_task_queue_runnable(task_queue *task_queue) noexcept
    : task_queue_(task_queue)
    {
    }

    void run_impl() override
    {
        thread_pool_task_handler handler;
        while(true)
        {
            {
                std::unique_lock<std::mutex> lock(task_queue_->lock_);
                if (task_queue_->queue_.empty())
                {
                    if (task_queue_->stop_)
                    {
                        break;
                    }
                    task_queue_->cond_.wait(lock, [this]()->bool
                    {
                        return task_queue_->queue_.size() || task_queue_->stop_;
                    });
                    if (task_queue_->queue_.empty() && task_queue_->stop_)
                    {
                        break;
                    }
                }
                handler = std::move(task_queue_->queue_.front());
                task_queue_->queue_.pop();
            }
            task_queue_->cond_.notify_all();
            handler();
        }
    }

private:
    task_queue *task_queue_;
};

class thread_pool_task_queue final
: public task_queue, public thread_pool<thread_pool_task_queue_runnable, task_queue *>
{
public:
    thread_pool_task_queue(int thr_num)
    : task_queue(), thread_pool<thread_pool_task_queue_runnable, task_queue *>(thr_num, this)
    {
    }

    thread_pool_task_queue(const thread_pool_task_queue &) = delete;
    thread_pool_task_queue &operator=(const thread_pool_task_queue &) = delete;
    thread_pool_task_queue(thread_pool_task_queue &&) = delete;
    thread_pool_task_queue &operator=(thread_pool_task_queue &&) = delete;

    ~thread_pool_task_queue() = default;

    void stop() noexcept
    {
        {
            std::unique_lock<std::mutex> lock(lock_);
            stop_ = true;
            cond_.notify_all();
        }
        join();
    }
};

}   // namespace task_queue

using thread_pool_task_queue = task_queue::thread_pool_task_queue;

using thread_pool_task_handler = task_queue::thread_pool_task_handler;

}   // namespace cppev

#endif  // thread_pool.h
