#ifndef _lock_h_6C0224787A17_
#define _lock_h_6C0224787A17_

#include <vector>
#include <mutex>
#include <pthread.h>
#include <atomic>
#include "cppev/utils.h"

namespace cppev
{

class spinlock;
class pshared_lock;
class pshared_cond;
class pshared_one_time_fence;
class pshared_barrier;
class pshared_rwlock;
class rdlockguard;
class wrlockguard;

#ifdef __linux__
// pthread implementation is about two times faster than atomic implementation
#define CPPEV_SPINLOCK_USE_PTHREAD
#endif

// Usage of spinlock is usually not recommended. Only used when task that shall be protected by lock is
// really important and performance sensitive and very simple.
class spinlock final
{
public:
    spinlock()
    {
#ifdef CPPEV_SPINLOCK_USE_PTHREAD
        int ret = pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
        if (ret != 0)
        {
            throw_system_error("pthread_spin_init error", ret);
        }
#else
        lock_.clear(std::memory_order_release);
#endif
    }

    spinlock(const spinlock &&) = delete;
    spinlock &operator=(const spinlock &&) = delete;
    spinlock(spinlock &&) = delete;
    spinlock &operator=(spinlock &&) = delete;

    ~spinlock() noexcept
    {
#ifdef CPPEV_SPINLOCK_USE_PTHREAD
        pthread_spin_destroy(&lock_);
#endif
    }

    void lock()
    {
#ifdef CPPEV_SPINLOCK_USE_PTHREAD
        int ret = pthread_spin_lock(&lock_);
        if (ret != 0)
        {
            throw_system_error("pthread_spin_lock error", ret);
        }
#else
        while (lock_.test_and_set(std::memory_order_acq_rel)) ;
#endif
    }

    void unlock()
    {
#ifdef CPPEV_SPINLOCK_USE_PTHREAD
        int ret = pthread_spin_unlock(&lock_);
        if (ret != 0)
        {
            throw_system_error("pthread_spin_unlock error", ret);
        }
#else
        lock_.clear(std::memory_order_release);
#endif
    }

    bool trylock()
    {
#ifdef CPPEV_SPINLOCK_USE_PTHREAD
        int ret = pthread_spin_trylock(&lock_);
        if (ret == 0)
        {
            return true;
        }
        else if (ret == EBUSY)
        {
            return false;
        }
        else
        {
            throw_system_error("pthread_spin_trylock error", ret);
        }
        return true;
#else
        return !lock_.test_and_set(std::memory_order_acq_rel);
#endif
    }

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
    pshared_lock()
    {
        int ret = 0;
        pthread_mutexattr_t attr;
        ret = pthread_mutexattr_init(&attr);
        if (ret != 0)
        {
            throw_system_error("pthread_mutexattr_init error", ret);
        }
        ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        if (ret != 0)
        {
            throw_system_error("pthread_mutexattr_settype error", ret);
        }
        ret = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE);
        if (ret != 0)
        {
            throw_system_error("pthread_mutexattr_setprotocol error", ret);
        }
        ret = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        if (ret != 0)
        {
            throw_system_error("pthread_mutexattr_setpshared error", ret);
        }
        ret = pthread_mutex_init(&lock_, &attr);
        if (ret != 0)
        {
            throw_system_error("pthread_mutex_init error", ret);
        }
        ret = pthread_mutexattr_destroy(&attr);
        if (ret != 0)
        {
            throw_system_error("pthread_mutexattr_destroy error", ret);
        }
    }

    pshared_lock(const pshared_lock &) = delete;
    pshared_lock &operator=(const pshared_lock &) = delete;
    pshared_lock(pshared_lock &&) = delete;
    pshared_lock &operator=(pshared_lock &&) = delete;

    ~pshared_lock() noexcept
    {
        pthread_mutex_destroy(&lock_);
    }

    void lock()
    {
        int ret = pthread_mutex_lock(&lock_);
        if (ret != 0)
        {
            throw_system_error("pthread_mutex_lock error", ret);
        }
    }

    bool try_lock()
    {
        int ret = pthread_mutex_trylock(&lock_);
        if (ret != 0)
        {
            if (ret == EBUSY)
            {
                return false;
            }
            throw_system_error("pthread_mutex_trylock error", ret);
        }
        return true;
    }

    void unlock()
    {
        int ret = pthread_mutex_unlock(&lock_);
        if (ret != 0)
        {
            throw_system_error("pthread_mutex_unlock error", ret);
        }
    }

private:
    pthread_mutex_t lock_;
};

class pshared_cond final
{
public:
    using condition = std::function<bool()>;

    pshared_cond()
    {
        int ret = 0;
        pthread_condattr_t attr;
        ret = pthread_condattr_init(&attr);
        if (ret != 0)
        {
            throw_system_error("pthread_condattr_init error", ret);
        }
        ret = pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        if (ret != 0)
        {
            throw_system_error("pthread_condattr_setpshared error", ret);
        }
        ret = pthread_cond_init(&cond_, &attr);
        if (ret != 0)
        {
            throw_system_error("pthread_cond_init error", ret);
        }
        ret = pthread_condattr_destroy(&attr);
        if (ret != 0)
        {
            throw_system_error("pthread_condattr_destroy error", ret);
        }
    }

    pshared_cond(const pshared_cond &) = delete;
    pshared_cond &operator=(const pshared_cond &) = delete;
    pshared_cond(pshared_cond &&) = delete;
    pshared_cond &operator=(pshared_cond &&) = delete;

    ~pshared_cond() noexcept
    {
        pthread_cond_destroy(&cond_);
    }

    void wait(std::unique_lock<pshared_lock> &lock)
    {
        int ret = pthread_cond_wait(&cond_, &lock.mutex()->lock_);
        if (ret != 0)
        {
            throw_system_error("pthread_cond_wait error", ret);
        }
    }

    void wait(std::unique_lock<pshared_lock> &lock, const condition &cond)
    {
        while (!cond())
        {
            wait(lock);
        }
    }

    void notify_one()
    {
        int ret = pthread_cond_signal(&cond_);
        if (ret != 0)
        {
            throw_system_error("pthread_cond_signal error", ret);
        }
    }

    void notify_all()
    {
        int ret = pthread_cond_broadcast(&cond_);
        if (ret != 0)
        {
            throw_system_error("pthread_cond_broadcast error", ret);
        }
    }

private:
    pthread_cond_t cond_;
};

class pshared_one_time_fence final
{
public:
    pshared_one_time_fence()
    : ok_(false)
    {
    }

    pshared_one_time_fence(const pshared_one_time_fence &) = delete;
    pshared_one_time_fence &operator=(const pshared_one_time_fence &) = delete;
    pshared_one_time_fence(pshared_one_time_fence &&) = delete;
    pshared_one_time_fence &operator=(pshared_one_time_fence &&) = delete;

    ~pshared_one_time_fence() = default;

    void wait()
    {
        if (!ok_)
        {
            std::unique_lock<pshared_lock> lock(lock_);
            if (!ok_)
            {
                cond_.wait(lock, [this](){ return ok_; });
            }
        }
    }

    void notify()
    {
        if (!ok_)
        {
            std::unique_lock<pshared_lock> lock(lock_);
            ok_ = true;
            cond_.notify_one();
        }
    }

    bool ok() const noexcept
    {
        return ok_;
    }

private:
    bool ok_;

    pshared_lock lock_;

    pshared_cond cond_;
};

class pshared_barrier final
{
public:
    pshared_barrier(int count)
    : count_(count)
    {
    }

    pshared_barrier(const pshared_barrier &) = delete;
    pshared_barrier &operator=(const pshared_barrier &) = delete;
    pshared_barrier(pshared_barrier &&) = delete;
    pshared_barrier &operator=(pshared_barrier &&) = delete;

    ~pshared_barrier() = default;

    void wait()
    {
        std::unique_lock<pshared_lock> lock(lock_);
        --count_;
        if (count_ == 0)
        {
            cond_.notify_all();
        }
        else if (count_ > 0)
        {
            cond_.wait(lock, [this]() { return count_ == 0; });
        }
        else
        {
            throw_logic_error("too many threads waited in the barrier");
        }
    }

private:
    int count_;

    pshared_lock lock_;

    pshared_cond cond_;
};

class pshared_rwlock final
{
public:
    pshared_rwlock()
    {
        int ret = 0;
        pthread_rwlockattr_t attr;
        ret = pthread_rwlockattr_init(&attr);
        if (ret != 0)
        {
            throw_system_error("pthread_rwlockattr_init error", ret);
        }
        ret = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        if (ret != 0)
        {
            throw_system_error("pthread_rwlockattr_setpshared error", ret);
        }
        ret = pthread_rwlock_init(&lock_, &attr);
        if (ret != 0)
        {
            throw_system_error("pthread_rwlock_init error", ret);
        }
        ret = pthread_rwlockattr_destroy(&attr);
        if (ret != 0)
        {
            throw_system_error("pthread_rwlockattr_destroy error", ret);
        }
    }

    pshared_rwlock(const pshared_rwlock &) = delete;
    pshared_rwlock &operator=(const pshared_rwlock &) = delete;
    pshared_rwlock(pshared_rwlock &&) = delete;
    pshared_rwlock &operator=(pshared_rwlock &&) = delete;

    ~pshared_rwlock() noexcept
    {
        pthread_rwlock_destroy(&lock_);
    }

    void unlock()
    {
        int ret = pthread_rwlock_unlock(&lock_);
        if (ret != 0)
        {
            throw_system_error("pthread_rwlock_unlock error", ret);
        }
    }

    void rdlock()
    {
        int ret = pthread_rwlock_rdlock(&lock_);
        if (ret != 0)
        {
            throw_system_error("pthread_rwlock_rdlock error", ret);
        }
    }

    void wrlock()
    {
        int ret = pthread_rwlock_wrlock(&lock_);
        if (ret != 0)
        {
            throw_system_error("pthread_rwlock_wrlock error", ret);
        }
    }

    bool try_rdlock()
    {
        int ret = pthread_rwlock_tryrdlock(&lock_);
        if (ret == 0)
        {
            return true;
        }
        else if (ret == EBUSY || ret == EAGAIN)
        {
            return false;
        }
        throw_system_error("pthread_rwlock_tryrdlock error", ret);
        return ret;
    }

    bool try_wrlock()
    {
        int ret = pthread_rwlock_trywrlock(&lock_);
        if (ret == 0)
        {
            return true;
        }
        else if (ret == EBUSY)
        {
            return false;
        }
        throw_system_error("pthread_rwlock_trywrlock error", ret);
        return ret;
    }

private:
    pthread_rwlock_t lock_;
};

class rdlockguard final
{
public:
    explicit rdlockguard(pshared_rwlock &lock)
    : rwlock_(&lock)
    {
        rwlock_->rdlock();
    }

    rdlockguard(const rdlockguard &) = delete;
    rdlockguard &operator=(const rdlockguard &) = delete;

    rdlockguard(rdlockguard &&other) noexcept
    {
        this->rwlock_ = other.rwlock_;
        other.rwlock_ = nullptr;
    }

    rdlockguard &operator=(rdlockguard &&other) noexcept
    {
        this->rwlock_ = other.rwlock_;
        other.rwlock_ = nullptr;

        return *this;
    }

    ~rdlockguard() noexcept
    {
        if (rwlock_ != nullptr)
        {
            try
            {
                rwlock_->unlock();
            }
            catch(...)
            {
            }
        }
    }

    void lock()
    {
        rwlock_->rdlock();
    }

    void unlock()
    {
        rwlock_->unlock();
    }

private:
    pshared_rwlock *rwlock_;
};

class wrlockguard final
{
public:
    explicit wrlockguard(pshared_rwlock &lock)
    : rwlock_(&lock)
    {
        rwlock_->wrlock();
    }

    wrlockguard(const wrlockguard &) = delete;
    wrlockguard &operator=(const wrlockguard &) = delete;

    wrlockguard(wrlockguard &&other) noexcept
    {
        this->rwlock_ = other.rwlock_;
        other.rwlock_ = nullptr;
    }

    wrlockguard &operator=(wrlockguard &&other) noexcept
    {
        this->rwlock_ = other.rwlock_;
        other.rwlock_ = nullptr;

        return *this;
    }

    ~wrlockguard() noexcept
    {
        if (rwlock_ != nullptr)
        {
            try
            {
                rwlock_->unlock();
            }
            catch(...)
            {
            }
        }
    }

    void lock()
    {
        rwlock_->wrlock();
    }

    void unlock()
    {
        rwlock_->unlock();
    }

private:
    pshared_rwlock *rwlock_;
};

}   // namespace cppev

#endif  // lock.h
