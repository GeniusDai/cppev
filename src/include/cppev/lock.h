#ifndef _cppev_lock_h_6C0224787A17_
#define _cppev_lock_h_6C0224787A17_

#include <pthread.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <vector>

#include "cppev/common.h"
#include "cppev/utils.h"

namespace cppev
{

enum class sync_level
{
    thread,
    process,
};

class CPPEV_PUBLIC mutex final
{
    friend class cond;

public:
    mutex(sync_level sl);

    mutex(const mutex &) = delete;
    mutex &operator=(const mutex &) = delete;
    mutex(mutex &&) = delete;
    mutex &operator=(mutex &&) = delete;

    ~mutex() noexcept;

    void lock();

    bool try_lock();

    void unlock();

private:
    pthread_mutex_t lock_;
};

class CPPEV_PUBLIC cond final
{
public:
    using predicate = std::function<bool()>;

    cond(sync_level sl);

    cond(const cond &) = delete;
    cond &operator=(const cond &) = delete;
    cond(cond &&) = delete;
    cond &operator=(cond &&) = delete;

    ~cond() noexcept;

    void wait(std::unique_lock<mutex> &lock);

    void wait(std::unique_lock<mutex> &lock, const predicate &pred);

    template <class Rep, class Period>
    std::cv_status wait_for(std::unique_lock<mutex> &lock,
                            const std::chrono::duration<Rep, Period> &rel_time)
    {
        return wait_until(lock, std::chrono::steady_clock::now() + rel_time);
    }

    template <class Rep, class Period>
    bool wait_for(std::unique_lock<mutex> &lock,
                  const std::chrono::duration<Rep, Period> &rel_time,
                  const predicate &pred)
    {
        return wait_until(lock, std::chrono::steady_clock::now() + rel_time,
                          pred);
    }

    template <class Duration>
    std::cv_status wait_until(
        std::unique_lock<mutex> &lock,
        const std::chrono::time_point<std::chrono::system_clock, Duration>
            &abs_time)
    {
        auto n_abs_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              abs_time.time_since_epoch())
                              .count();
        timespec ts;
        ts.tv_sec = n_abs_time / 1'000'000'000;
        ts.tv_nsec = n_abs_time % 1'000'000'000;

        // The implementation uses system clock to align with the standard
        // library.
        int ret = pthread_cond_timedwait(&cond_, &lock.mutex()->lock_, &ts);
        std::cv_status status = std::cv_status::no_timeout;
        if (ret != 0)
        {
            if (ret == EINVAL)
            {
                throw_system_error_with_specific_errno(
                    "pthread_cond_wait error", ret);
            }
            else if (ret == ETIMEDOUT)
            {
                status = std::cv_status::timeout;
            }
        }
        return status;
    }

    template <class Clock, class Duration>
    std::cv_status wait_until(
        std::unique_lock<mutex> &lock,
        const std::chrono::time_point<Clock, Duration> &abs_time)
    {
        auto sys_abs_time =
            std::chrono::system_clock::now() + (abs_time - Clock::now());
        return wait_until(lock, sys_abs_time);
    }

    template <class Clock, class Duration>
    bool wait_until(std::unique_lock<mutex> &lock,
                    const std::chrono::time_point<Clock, Duration> &abs_time,
                    predicate pred)
    {
        while (!pred())
        {
            if (wait_until(lock, abs_time) == std::cv_status::timeout)
            {
                return pred();
            }
        }
        return true;
    }

    void notify_one();

    void notify_all();

private:
    pthread_cond_t cond_;
};

class CPPEV_PUBLIC one_time_fence final
{
public:
    one_time_fence(sync_level sl);

    one_time_fence(const one_time_fence &) = delete;
    one_time_fence &operator=(const one_time_fence &) = delete;
    one_time_fence(one_time_fence &&) = delete;
    one_time_fence &operator=(one_time_fence &&) = delete;

    ~one_time_fence();

    void wait();

    void notify();

private:
    bool ok_;

    mutex lock_;

    cond cond_;
};

class CPPEV_PUBLIC barrier final
{
public:
    barrier(sync_level sl, int count);

    barrier(const barrier &) = delete;
    barrier &operator=(const barrier &) = delete;
    barrier(barrier &&) = delete;
    barrier &operator=(barrier &&) = delete;

    ~barrier();

    void wait();

private:
    int count_;

    mutex lock_;

    cond cond_;
};

class CPPEV_PUBLIC rwlock final
{
public:
    rwlock(sync_level sl);

    rwlock(const rwlock &) = delete;
    rwlock &operator=(const rwlock &) = delete;
    rwlock(rwlock &&) = delete;
    rwlock &operator=(rwlock &&) = delete;

    ~rwlock() noexcept;

    void unlock();

    void rdlock();

    void wrlock();

    bool try_rdlock();

    bool try_wrlock();

private:
    pthread_rwlock_t lock_;
};

class CPPEV_PUBLIC rdlockguard final
{
public:
    explicit rdlockguard(rwlock &lock);

    rdlockguard(const rdlockguard &) = delete;
    rdlockguard &operator=(const rdlockguard &) = delete;
    rdlockguard(rdlockguard &&other) noexcept;
    rdlockguard &operator=(rdlockguard &&other) noexcept;

    ~rdlockguard() noexcept;

    void lock();

    void unlock();

private:
    rwlock *rwlock_;
};

class CPPEV_PUBLIC wrlockguard final
{
public:
    explicit wrlockguard(rwlock &lock);

    wrlockguard(const wrlockguard &) = delete;
    wrlockguard &operator=(const wrlockguard &) = delete;
    wrlockguard(wrlockguard &&other) noexcept;
    wrlockguard &operator=(wrlockguard &&other) noexcept;

    ~wrlockguard() noexcept;

    void lock();

    void unlock();

private:
    rwlock *rwlock_;
};

}  // namespace cppev

#endif  // lock.h
