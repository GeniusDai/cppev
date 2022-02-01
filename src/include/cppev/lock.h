#ifndef _lock_h_6C0224787A17_
#define _lock_h_6C0224787A17_

#include "cppev/common_utils.h"
#include <pthread.h>
#include <exception>
#include <unordered_set>
#include <mutex>
#include <condition_variable>

namespace cppev
{

// spinlock for linux
#if _POSIX_C_SOURCE >= 200112L

class spinlock final
: public uncopyable
{
public:
    spinlock()
    {
        if (pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE) != 0)
        {
            throw_system_error("pthread_spin_init error");
        }
    }

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
        bool ret = pthread_spin_trylock(&lock_);
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
: public uncopyable
{
public:
    rwlock()
    {
        if (pthread_rwlock_init(&lock_, nullptr) != 0)
        {
            throw_system_error("pthread_rwlock_init error");
        }
    }

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
: public uncopyable
{
public:
    rdlockguard(rwlock &lock)
    {
        rwlock_ = &lock;
        rwlock_->rdlock();
    }

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
: public uncopyable
{
public:
    wrlockguard(rwlock &lock)
    {
        rwlock_ = &lock;
        rwlock_->wrlock();
    }

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