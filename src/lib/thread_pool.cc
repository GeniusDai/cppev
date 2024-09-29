#include "cppev/thread_pool.h"

namespace cppev
{

namespace task_queue
{

task_queue::task_queue() noexcept
: stop_(false)
{
}

void task_queue::add_task(const thread_pool_task_handler &h) noexcept
{
    std::unique_lock<std::mutex> lock(lock_);
    queue_.push(h);
    cond_.notify_one();
}

void task_queue::add_task(thread_pool_task_handler &&h) noexcept
{
    std::unique_lock<std::mutex> lock(lock_);
    queue_.push(std::forward<thread_pool_task_handler>(h));
    cond_.notify_one();
}

void task_queue::add_task(const std::vector<thread_pool_task_handler> &vh) noexcept
{
    std::unique_lock<std::mutex> lock(lock_);
    for (const auto &h : vh)
    {
        queue_.push(h);
    }
    cond_.notify_all();
}


void thread_pool_task_queue_runnable::run_impl()
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


thread_pool_task_queue::thread_pool_task_queue(int thr_num)
: task_queue(), thread_pool<thread_pool_task_queue_runnable, task_queue *>(thr_num, this)
{
}

void thread_pool_task_queue::stop() noexcept
{
    {
        std::unique_lock<std::mutex> lock(lock_);
        stop_ = true;
        cond_.notify_all();
    }
    join();
}

}   // namespace task_queue

}   // namespace cppev
