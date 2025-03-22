#ifndef _cppev_thread_pool_h_6C0224787A17_
#define _cppev_thread_pool_h_6C0224787A17_

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <type_traits>
#include <vector>

#include "cppev/runnable.h"
#include "cppev/utils.h"

namespace cppev
{

template <typename Runnable, typename... Args>
class CPPEV_PUBLIC thread_pool
{
    static_assert(std::is_base_of<runnable, Runnable>::value, "Not runnable");
    static_assert(std::is_constructible<Runnable, Args &&...>::value,
                  "Not constructible");

public:
    using container_type = std::vector<std::unique_ptr<Runnable>>;

    explicit thread_pool(int thr_num, Args &&...args)
    {
        for (int i = 0; i < thr_num; ++i)
        {
            thrs_.push_back(
                std::make_unique<Runnable>(std::forward<Args>(args)...));
        }
    }

    thread_pool(const thread_pool &) = delete;
    thread_pool &operator=(const thread_pool &) = delete;

    thread_pool(thread_pool &&other) = default;
    thread_pool &operator=(thread_pool &&other) = default;

    virtual ~thread_pool() = default;

    class iterator
    {
    public:
        explicit iterator(container_type &rv, int idx) : rv_(rv), idx_(idx)
        {
        }

        bool operator!=(const iterator &other)
        {
            return idx_ != other.idx_;
        }

        Runnable &operator*()
        {
            return *rv_[idx_];
        }

        iterator &operator++()
        {
            ++idx_;
            return *this;
        }

    private:
        container_type &rv_;

        int idx_;
    };

    iterator begin()
    {
        return iterator(thrs_, 0);
    }

    iterator end()
    {
        return iterator(thrs_, thrs_.size());
    }

    // Run all threads.
    void run()
    {
        for (auto &thr : thrs_)
        {
            thr->run();
        }
    }

    // Wait for all threads.
    void join()
    {
        for (auto &thr : thrs_)
        {
            thr->join();
        }
    }

    // Cancel all threads.
    virtual void cancel()
    {
        for (auto &thr : thrs_)
        {
            thr->cancel();
        }
    }

    // Specific const one of the threads.
    const Runnable &operator[](int i) const noexcept
    {
        return *(thrs_[i].get());
    }

    // Specific one of the threads.
    Runnable &operator[](int i) noexcept
    {
        return *(thrs_[i].get());
    }

    // Thread pool size.
    int size() const noexcept
    {
        return thrs_.size();
    }

protected:
    container_type thrs_;
};

class thread_pool_task_queue;

class CPPEV_PRIVATE thread_pool_task_queue_runnable final : public runnable
{
public:
    thread_pool_task_queue_runnable(thread_pool_task_queue *tptq) noexcept;

    void run_impl() override;

private:
    thread_pool_task_queue *tptq_;
};

using thread_pool_task_handler = std::function<void(void)>;

class CPPEV_PUBLIC thread_pool_task_queue final
    : private thread_pool<thread_pool_task_queue_runnable,
                          thread_pool_task_queue *>
{
    friend class thread_pool_task_queue_runnable;

    using thread_pool_base_type =
        thread_pool<thread_pool_task_queue_runnable, thread_pool_task_queue *>;

public:
    thread_pool_task_queue(int thr_num);

    thread_pool_task_queue(const thread_pool_task_queue &) = delete;
    thread_pool_task_queue &operator=(const thread_pool_task_queue &) = delete;
    thread_pool_task_queue(thread_pool_task_queue &&) = delete;
    thread_pool_task_queue &operator=(thread_pool_task_queue &&) = delete;

    ~thread_pool_task_queue();

    using thread_pool_base_type::run;

    using thread_pool_base_type::size;

    void add_task(const thread_pool_task_handler &h) noexcept;

    void add_task(thread_pool_task_handler &&h) noexcept;

    void add_task(const std::vector<thread_pool_task_handler> &vh) noexcept;

    void stop() noexcept;

private:
    std::queue<thread_pool_task_handler> queue_;

    std::mutex lock_;

    std::condition_variable cond_;

    bool stop_;
};

}  // namespace cppev

#endif  // thread_pool.h
