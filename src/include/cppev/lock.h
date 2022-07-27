#ifndef _lock_h_6C0224787A17_
#define _lock_h_6C0224787A17_

#include "cppev/common_utils.h"
#include <pthread.h>

namespace cppev
{

#if _POSIX_C_SOURCE >= 200112L

class spinlock final
{
public:
    spinlock()
    {
        if (pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE) != 0)
        {
            throw_system_error("pthread_spin_init error");
        }
    }

    spinlock(const spinlock &&) = delete;
    spinlock &operator=(const spinlock &&) = delete;
    spinlock(spinlock &&) = delete;
    spinlock &operator=(spinlock &&) = delete;

    ~spinlock()
    {
        if (pthread_spin_destroy(&lock_) != 0)
        {
            throw_system_error("pthread_spin_destroy error");
        }
    }

    void lock()
    {
        if (pthread_spin_lock(&lock_) != 0)
        {
            throw_system_error("pthread_spin_lock error");
        }
    }

    void unlock()
    {
        if (pthread_spin_unlock(&lock_) != 0)
        {
            throw_system_error("pthread_spin_unlock error");
        }
    }

    bool trylock()
    {
        int ret = pthread_spin_trylock(&lock_);
        if (ret == 0)
        {
            return true;
        }
        else if (ret == EBUSY)
        {
            return false;
        }
        throw_system_error("pthread_spin_trylock error");
        return ret;
    }

private:
    pthread_spinlock_t lock_;
};

#endif  // spinlock

class rwlock final
{
public:
    rwlock()
    {
#ifdef __linux__
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
#else
        if (pthread_rwlock_init(&lock_, nullptr) != 0)
#endif  // __linux__
        {
            throw_system_error("pthread_rwlock_init error");
        }
    }

    rwlock(const rwlock &) = delete;
    rwlock &operator=(const rwlock &) = delete;
    rwlock(rwlock &&) = delete;
    rwlock &operator=(rwlock &&) = delete;

    ~rwlock()
    {
        if (pthread_rwlock_destroy(&lock_) != 0)
        {
            throw_system_error("pthread_rwlock_destroy error");
        }
    }

    void unlock()
    {
        if (::pthread_rwlock_unlock(&lock_) != 0)
        {
            throw_system_error("pthread_rwlock_destroy error");
        }
    }

    void rdlock()
    {
        if (::pthread_rwlock_rdlock(&lock_) != 0)
        {
            throw_system_error("pthread_rwlock_rdlock error");
        }
    }

    void wrlock()
    {
        if (::pthread_rwlock_wrlock(&lock_) != 0)
        {
            throw_system_error("pthread_rwlock_wrlock error");
        }
    }

    bool try_rdlock()
    {
        int ret = ::pthread_rwlock_tryrdlock(&lock_);
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
        int ret = ::pthread_rwlock_trywrlock(&lock_);
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
    explicit rdlockguard(rwlock &lock)
    {
        rwlock_ = &lock;
        rwlock_->rdlock();
    }

    rdlockguard(const rdlockguard &) = delete;
    rdlockguard &operator=(const rdlockguard &) = delete;
    rdlockguard(rdlockguard &&) = delete;
    rdlockguard &operator=(rdlockguard &&) = delete;

    ~rdlockguard()
    {
        rwlock_->unlock();
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
    rwlock *rwlock_;
};

class wrlockguard final
{
public:
    explicit wrlockguard(rwlock &lock)
    {
        rwlock_ = &lock;
        rwlock_->wrlock();
    }

    wrlockguard(const wrlockguard &) = delete;
    wrlockguard &operator=(const wrlockguard &) = delete;
    wrlockguard(wrlockguard &&) = delete;
    wrlockguard &operator=(wrlockguard &&) = delete;

    ~wrlockguard()
    {
        rwlock_->unlock();
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
    rwlock *rwlock_;
};

}   // namespace cppev

#endif  // lock.h
