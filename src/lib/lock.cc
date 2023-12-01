#include "cppev/lock.h"

namespace cppev
{

spinlock::spinlock()
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

spinlock::~spinlock() noexcept
{
#ifdef CPPEV_SPINLOCK_USE_PTHREAD
    pthread_spin_destroy(&lock_);
#endif
}

void spinlock::lock()
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

void spinlock::unlock()
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

bool spinlock::trylock()
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


pshared_lock::pshared_lock()
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

pshared_lock::~pshared_lock() noexcept
{
    pthread_mutex_destroy(&lock_);
}

void pshared_lock::lock()
{
    int ret = pthread_mutex_lock(&lock_);
    if (ret != 0)
    {
        throw_system_error("pthread_mutex_lock error", ret);
    }
}

bool pshared_lock::try_lock()
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

void pshared_lock::unlock()
{
    int ret = pthread_mutex_unlock(&lock_);
    if (ret != 0)
    {
        throw_system_error("pthread_mutex_unlock error", ret);
    }
}


pshared_cond::pshared_cond()
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

pshared_cond::~pshared_cond() noexcept
{
    pthread_cond_destroy(&cond_);
}

void pshared_cond::wait(std::unique_lock<pshared_lock> &lock)
{
    int ret = pthread_cond_wait(&cond_, &lock.mutex()->lock_);
    if (ret != 0)
    {
        throw_system_error("pthread_cond_wait error", ret);
    }
}

void pshared_cond::wait(std::unique_lock<pshared_lock> &lock, const condition &cond)
{
    while (!cond())
    {
        wait(lock);
    }
}

void pshared_cond::notify_one()
{
    int ret = pthread_cond_signal(&cond_);
    if (ret != 0)
    {
        throw_system_error("pthread_cond_signal error", ret);
    }
}

void pshared_cond::notify_all()
{
    int ret = pthread_cond_broadcast(&cond_);
    if (ret != 0)
    {
        throw_system_error("pthread_cond_broadcast error", ret);
    }
}


pshared_one_time_fence::pshared_one_time_fence()
: ok_(false)
{
}

void pshared_one_time_fence::wait()
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

void pshared_one_time_fence::notify()
{
    if (!ok_)
    {
        std::unique_lock<pshared_lock> lock(lock_);
        ok_ = true;
        cond_.notify_one();
    }
}

bool pshared_one_time_fence::ok() const noexcept
{
    return ok_;
}


pshared_barrier::pshared_barrier(int count)
: count_(count)
{
}

void pshared_barrier::wait()
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


pshared_rwlock::pshared_rwlock()
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

pshared_rwlock::~pshared_rwlock() noexcept
{
    pthread_rwlock_destroy(&lock_);
}

void pshared_rwlock::unlock()
{
    int ret = pthread_rwlock_unlock(&lock_);
    if (ret != 0)
    {
        throw_system_error("pthread_rwlock_unlock error", ret);
    }
}

void pshared_rwlock::rdlock()
{
    int ret = pthread_rwlock_rdlock(&lock_);
    if (ret != 0)
    {
        throw_system_error("pthread_rwlock_rdlock error", ret);
    }
}

void pshared_rwlock::wrlock()
{
    int ret = pthread_rwlock_wrlock(&lock_);
    if (ret != 0)
    {
        throw_system_error("pthread_rwlock_wrlock error", ret);
    }
}

bool pshared_rwlock::try_rdlock()
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

bool pshared_rwlock::try_wrlock()
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


rdlockguard::rdlockguard(pshared_rwlock &lock)
: rwlock_(&lock)
{
    rwlock_->rdlock();
}

rdlockguard::rdlockguard(rdlockguard &&other) noexcept
{
    this->rwlock_ = other.rwlock_;
    other.rwlock_ = nullptr;
}

rdlockguard &rdlockguard::operator=(rdlockguard &&other) noexcept
{
    this->rwlock_ = other.rwlock_;
    other.rwlock_ = nullptr;

    return *this;
}

rdlockguard::~rdlockguard() noexcept
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

void rdlockguard::lock()
{
    rwlock_->rdlock();
}

void rdlockguard::unlock()
{
    rwlock_->unlock();
}


wrlockguard::wrlockguard(pshared_rwlock &lock)
: rwlock_(&lock)
{
    rwlock_->wrlock();
}

wrlockguard::wrlockguard(wrlockguard &&other) noexcept
{
    this->rwlock_ = other.rwlock_;
    other.rwlock_ = nullptr;
}

wrlockguard &wrlockguard::operator=(wrlockguard &&other) noexcept
{
    this->rwlock_ = other.rwlock_;
    other.rwlock_ = nullptr;

    return *this;
}

wrlockguard::~wrlockguard() noexcept
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

void wrlockguard::lock()
{
    rwlock_->wrlock();
}

void wrlockguard::unlock()
{
    rwlock_->unlock();
}

}   // namespace cppev