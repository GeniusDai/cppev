#include "cppev/thread_pool.h"

namespace cppev
{

thread_pool_task_queue_runnable::thread_pool_task_queue_runnable(
    thread_pool_task_queue *tptq) noexcept
    : tptq_(tptq)
{
}

void thread_pool_task_queue_runnable::run_impl()
{
    thread_pool_task_handler handler;
    while (true)
    {
        {
            std::unique_lock<std::mutex> lock(tptq_->lock_);
            if (tptq_->queue_.empty())
            {
                if (tptq_->stop_)
                {
                    break;
                }
                tptq_->cond_.wait(
                    lock, [this]() -> bool
                    { return tptq_->queue_.size() || tptq_->stop_; });
            }
            if (tptq_->queue_.empty() && tptq_->stop_)
            {
                break;
            }
            handler = std::move(tptq_->queue_.front());
            tptq_->queue_.pop();
        }
        tptq_->cond_.notify_all();
        handler();
    }
}

thread_pool_task_queue::thread_pool_task_queue(int thr_num)
    : thread_pool<thread_pool_task_queue_runnable, thread_pool_task_queue *>(
          thr_num, this),
      stop_(false)
{
}

thread_pool_task_queue::~thread_pool_task_queue() = default;

void thread_pool_task_queue::add_task(
    const thread_pool_task_handler &h) noexcept
{
    std::unique_lock<std::mutex> lock(lock_);
    queue_.push(h);
    cond_.notify_one();
}

void thread_pool_task_queue::add_task(thread_pool_task_handler &&h) noexcept
{
    std::unique_lock<std::mutex> lock(lock_);
    queue_.push(std::forward<thread_pool_task_handler>(h));
    cond_.notify_one();
}

void thread_pool_task_queue::add_task(
    const std::vector<thread_pool_task_handler> &vh) noexcept
{
    std::unique_lock<std::mutex> lock(lock_);
    for (const auto &h : vh)
    {
        queue_.push(h);
    }
    cond_.notify_all();
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

}  // namespace cppev
