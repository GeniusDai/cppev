#ifndef _lock_h_6C0224787A17_
#define _lock_h_6C0224787A17_

#include <vector>
#include <mutex>
#include <pthread.h>
#include <atomic>
#include "cppev/utils.h"

namespace cppev
{

#ifdef __linux__
// pthread implementation is about two times faster than atomic implementation
#define CPPEV_SPINLOCK_USE_PTHREAD
#endif

// Usage of spinlock is usually not recommended. Only used when task that shall be protected by lock is
// really important and performance sensitive and very simple.
class spinlock final
{
public:
    spinlock();

    spinlock(const spinlock &&) = delete;
    spinlock &operator=(const spinlock &&) = delete;
    spinlock(spinlock &&) = delete;
    spinlock &operator=(spinlock &&) = delete;

    ~spinlock() noexcept;

    void lock();

    void unlock();

    bool trylock();

private:
#ifdef CPPEV_SPINLOCK_USE_PTHREAD
    pthread_spinlock_t lock_;
#else
    std::atomic_flag lock_;
#endif
};

class pshared_lock final
{
    friend class pshared_cond;
public:
    pshared_lock();

    pshared_lock(const pshared_lock &) = delete;
    pshared_lock &operator=(const pshared_lock &) = delete;
    pshared_lock(pshared_lock &&) = delete;
    pshared_lock &operator=(pshared_lock &&) = delete;

    ~pshared_lock() noexcept;

    void lock();

    bool try_lock();

    void unlock();

private:
    pthread_mutex_t lock_;
};

class pshared_cond final
{
public:
    using condition = std::function<bool()>;

    pshared_cond();

    pshared_cond(const pshared_cond &) = delete;
    pshared_cond &operator=(const pshared_cond &) = delete;
    pshared_cond(pshared_cond &&) = delete;
    pshared_cond &operator=(pshared_cond &&) = delete;

    ~pshared_cond() noexcept;

    void wait(std::unique_lock<pshared_lock> &lock);

    void wait(std::unique_lock<pshared_lock> &lock, const condition &cond);

    void notify_one();

    void notify_all();

private:
    pthread_cond_t cond_;
};

class pshared_one_time_fence final
{
public:
    pshared_one_time_fence();

    pshared_one_time_fence(const pshared_one_time_fence &) = delete;
    pshared_one_time_fence &operator=(const pshared_one_time_fence &) = delete;
    pshared_one_time_fence(pshared_one_time_fence &&) = delete;
    pshared_one_time_fence &operator=(pshared_one_time_fence &&) = delete;

    ~pshared_one_time_fence() = default;

    void wait();

    void notify();

    bool ok() const noexcept;

private:
    bool ok_;

    pshared_lock lock_;

    pshared_cond cond_;
};

class pshared_barrier final
{
public:
    pshared_barrier(int count);

    pshared_barrier(const pshared_barrier &) = delete;
    pshared_barrier &operator=(const pshared_barrier &) = delete;
    pshared_barrier(pshared_barrier &&) = delete;
    pshared_barrier &operator=(pshared_barrier &&) = delete;

    ~pshared_barrier() = default;

    void wait();

private:
    int count_;

    pshared_lock lock_;

    pshared_cond cond_;
};

class pshared_rwlock final
{
public:
    pshared_rwlock();

    pshared_rwlock(const pshared_rwlock &) = delete;
    pshared_rwlock &operator=(const pshared_rwlock &) = delete;
    pshared_rwlock(pshared_rwlock &&) = delete;
    pshared_rwlock &operator=(pshared_rwlock &&) = delete;

    ~pshared_rwlock() noexcept;

    void unlock();

    void rdlock();

    void wrlock();

    bool try_rdlock();

    bool try_wrlock();

private:
    pthread_rwlock_t lock_;
};

class rdlockguard final
{
public:
    explicit rdlockguard(pshared_rwlock &lock);

    rdlockguard(const rdlockguard &) = delete;
    rdlockguard &operator=(const rdlockguard &) = delete;
    rdlockguard(rdlockguard &&other) noexcept;
    rdlockguard &operator=(rdlockguard &&other) noexcept;

    ~rdlockguard() noexcept;

    void lock();

    void unlock();

private:
    pshared_rwlock *rwlock_;
};

class wrlockguard final
{
public:
    explicit wrlockguard(pshared_rwlock &lock);

    wrlockguard(const wrlockguard &) = delete;
    wrlockguard &operator=(const wrlockguard &) = delete;
    wrlockguard(wrlockguard &&other) noexcept;
    wrlockguard &operator=(wrlockguard &&other) noexcept;

    ~wrlockguard() noexcept;

    void lock();

    void unlock();

private:
    pshared_rwlock *rwlock_;
};

}   // namespace cppev

#endif  // lock.h
