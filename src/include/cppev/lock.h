#ifndef _lock_h_6C0224787A17_
#define _lock_h_6C0224787A17_

#include <vector>
#include <mutex>
#include <pthread.h>
#include "cppev/common_utils.h"

namespace cppev
{

class spinlock;
class pshared_lock;
class pshared_cond;
class pshared_rwlock;
class rdlockguard;
class wrlockguard;


class spinlock final
{
public:
    spinlock()
    {
#ifdef __linux__
        if (pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE) != 0)
        {
            throw_system_error("pthread_spin_init error");
        }
#else
        unlock();
#endif
    }

    spinlock(const spinlock &&) = delete;
    spinlock &operator=(const spinlock &&) = delete;
    spinlock(spinlock &&) = delete;
    spinlock &operator=(spinlock &&) = delete;

    ~spinlock() noexcept
    {
#ifdef __linux__
        pthread_spin_destroy(&lock_);
#endif
    }

    void lock()
    {
#ifdef __linux__
        if (pthread_spin_lock(&lock_) != 0)
        {
            throw_system_error("pthread_spin_lock error");
        }
#else
        while (lock_.test_and_set(std::memory_order_acquire)) ;
#endif
    }

    void unlock()
    {
#ifdef __linux__
        if (pthread_spin_unlock(&lock_) != 0)
        {
            throw_system_error("pthread_spin_unlock error");
        }
#else
        lock_.clear(std::memory_order_release);
#endif
    }

    bool trylock()
    {
#ifdef __linux__
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
            throw_system_error("pthread_spin_trylock error");
        }
        return true;
#else
        return !lock_.test_and_set(std::memory_order_acquire);
#endif
    }

private:
#ifdef __linux__
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
        pthread_mutexattr_t attr;
        if (pthread_mutexattr_init(&attr) != 0)
        {
            throw_system_error("pthread_mutexattr_init error");
        }
        if (pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0)
        {
            throw_system_error("pthread_mutexattr_setpshared error");
        }
        if (pthread_mutex_init(&lock_, &attr) != 0)
        {
            throw_system_error("pthread_mutex_init error");
        }
        if (pthread_mutexattr_destroy(&attr) != 0)
        {
            throw_system_error("pthread_mutexattr_destroy error");
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
        if (pthread_mutex_lock(&lock_) != 0)
        {
            throw_system_error("pthread_mutex_lock error");
        }
    }

    bool try_lock()
    {
        if (pthread_mutex_trylock(&lock_) != 0)
        {
            if (errno == EBUSY)
            {
                return false;
            }
            throw_system_error("pthread_mutex_trylock error");
        }
        return true;
    }

    void unlock()
    {
        if (pthread_mutex_unlock(&lock_) != 0)
        {
            throw_system_error("pthread_mutex_unlock error");
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
        pthread_condattr_t attr;
        if (pthread_condattr_init(&attr) != 0)
        {
            throw_system_error("pthread_condattr_init error");
        }
        if (pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0)
        {
            throw_system_error("pthread_condattr_setpshared error");
        }
        if (pthread_cond_init(&cond_, &attr) != 0)
        {
            throw_system_error("pthread_cond_init error");
        }
        if (pthread_condattr_destroy(&attr) != 0)
        {
            throw_system_error("pthread_condattr_destroy error");
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

    void wait(std::unique_lock<pshared_lock> &lock, const condition &cond = []{ return true; })
    {
        while (true)
        {
            if (pthread_cond_wait(&cond_, &lock.mutex()->lock_) != 0)
            {
                throw_system_error("pthread_cond_wait error");
            }
            if (cond())
            {
                break;
            }
        }
    }

    void notify_one()
    {
        if (pthread_cond_signal(&cond_) != 0)
        {
            throw_system_error("pthread_cond_signal error");
        }
    }

    void notify_all()
    {
        if (pthread_cond_broadcast(&cond_) != 0)
        {
            throw_system_error("pthread_cond_broadcast error");
        }
    }

private:
    pthread_cond_t cond_;
};

class pshared_rwlock final
{
public:
    pshared_rwlock()
    {
        pthread_rwlockattr_t attr;
        if (pthread_rwlockattr_init(&attr) != 0)
        {
            throw_system_error("pthread_rwlockattr_init error");
        }
        if (pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED) != 0)
        {
            throw_system_error("pthread_rwlockattr_setpshared error");
        }
        if (pthread_rwlock_init(&lock_, &attr) != 0)
        {
            throw_system_error("pthread_rwlock_init error");
        }
        if (pthread_rwlockattr_destroy(&attr) != 0)
        {
            throw_system_error("pthread_rwlockattr_destroy error");
        }
    }

    pshared_rwlock(const pshared_rwlock &) = delete;
    pshared_rwlock &operator=(const pshared_rwlock &) = delete;
    pshared_rwlock(pshared_rwlock &&) = delete;
    pshared_rwlock &operator=(pshared_rwlock &&) = delete;

    ~pshared_rwlock()
    {
        if (pthread_rwlock_destroy(&lock_) != 0)
        {
            throw_system_error("pthread_rwlock_destroy error");
        }
    }

    void unlock()
    {
        if (pthread_rwlock_unlock(&lock_) != 0)
        {
            throw_system_error("pthread_rwlock_destroy error");
        }
    }

    void rdlock()
    {
        if (pthread_rwlock_rdlock(&lock_) != 0)
        {
            throw_system_error("pthread_rwlock_rdlock error");
        }
    }

    void wrlock()
    {
        if (pthread_rwlock_wrlock(&lock_) != 0)
        {
            throw_system_error("pthread_rwlock_wrlock error");
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
        throw_system_error("pthread_rwlock_tryrdlock error");
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
        throw_system_error("pthread_rwlock_trywrlock error");
        return ret;
    }

private:
    pthread_rwlock_t lock_;
};

class rdlockguard final
{
public:
    explicit rdlockguard(pshared_rwlock &lock)
    {
        rwlock_ = &lock;
        rwlock_->rdlock();
    }

    rdlockguard(const rdlockguard &) = delete;
    rdlockguard &operator=(const rdlockguard &) = delete;
    rdlockguard(rdlockguard &&) = delete;
    rdlockguard &operator=(rdlockguard &&) = delete;

    ~rdlockguard() noexcept
    {
        try
        {
            rwlock_->unlock();
        }
        catch(...)
        {}
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
    {
        rwlock_ = &lock;
        rwlock_->wrlock();
    }

    wrlockguard(const wrlockguard &) = delete;
    wrlockguard &operator=(const wrlockguard &) = delete;
    wrlockguard(wrlockguard &&) = delete;
    wrlockguard &operator=(wrlockguard &&) = delete;

    ~wrlockguard() noexcept
    {
        try
        {
            rwlock_->unlock();
        }
        catch(...)
        {}
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
