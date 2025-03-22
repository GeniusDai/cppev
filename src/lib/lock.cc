#include "cppev/lock.h"

namespace cppev
{

static const std::unordered_map<sync_level, int> sync_level_map = {
    {sync_level::thread, PTHREAD_PROCESS_PRIVATE},
    {sync_level::process, PTHREAD_PROCESS_SHARED},
};

mutex::mutex(sync_level sl)
{
    int ret = 0;
    pthread_mutexattr_t attr;
    ret = pthread_mutexattr_init(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_mutexattr_init error",
                                               ret);
    }
    ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_mutexattr_settype error", ret);
    }
    ret = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_mutexattr_setprotocol error", ret);
    }
    ret = pthread_mutexattr_setpshared(&attr, sync_level_map.at(sl));
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_mutexattr_setpshared error", ret);
    }
    ret = pthread_mutex_init(&lock_, &attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_mutex_init error", ret);
    }
    ret = pthread_mutexattr_destroy(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_mutexattr_destroy error", ret);
    }
}

mutex::~mutex() noexcept
{
    pthread_mutex_destroy(&lock_);
}

void mutex::lock()
{
    int ret = pthread_mutex_lock(&lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_mutex_lock error", ret);
    }
}

bool mutex::try_lock()
{
    int ret = pthread_mutex_trylock(&lock_);
    if (ret != 0)
    {
        if (ret == EBUSY)
        {
            return false;
        }
        throw_system_error_with_specific_errno("pthread_mutex_trylock error",
                                               ret);
    }
    return true;
}

void mutex::unlock()
{
    int ret = pthread_mutex_unlock(&lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_mutex_unlock error",
                                               ret);
    }
}

cond::cond(sync_level sl)
{
    int ret = 0;
    pthread_condattr_t attr;
    ret = pthread_condattr_init(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_condattr_init error",
                                               ret);
    }
    ret = pthread_condattr_setpshared(&attr, sync_level_map.at(sl));
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_condattr_setpshared error", ret);
    }
    ret = pthread_cond_init(&cond_, &attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_cond_init error", ret);
    }
    ret = pthread_condattr_destroy(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_condattr_destroy error",
                                               ret);
    }
}

cond::~cond() noexcept
{
    pthread_cond_destroy(&cond_);
}

void cond::wait(std::unique_lock<mutex> &lock)
{
    int ret = pthread_cond_wait(&cond_, &lock.mutex()->lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_cond_wait error", ret);
    }
}

void cond::wait(std::unique_lock<mutex> &lock, const predicate &pred)
{
    while (!pred())
    {
        wait(lock);
    }
}

void cond::notify_one()
{
    int ret = pthread_cond_signal(&cond_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_cond_signal error",
                                               ret);
    }
}

void cond::notify_all()
{
    int ret = pthread_cond_broadcast(&cond_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_cond_broadcast error",
                                               ret);
    }
}

one_time_fence::one_time_fence(sync_level sl) : ok_(false), lock_(sl), cond_(sl)
{
}

one_time_fence::~one_time_fence() = default;

void one_time_fence::wait()
{
    if (!ok_)
    {
        std::unique_lock<mutex> lock(lock_);
        if (!ok_)
        {
            cond_.wait(lock, [this]() { return ok_; });
        }
    }
}

void one_time_fence::notify()
{
    if (!ok_)
    {
        std::unique_lock<mutex> lock(lock_);
        ok_ = true;
        cond_.notify_one();
    }
}

barrier::barrier(sync_level sl, int count) : count_(count), lock_(sl), cond_(sl)
{
}

barrier::~barrier() = default;

void barrier::wait()
{
    std::unique_lock<mutex> lock(lock_);
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
        throw_logic_error("too many threads waiting in the barrier");
    }
}

rwlock::rwlock(sync_level sl)
{
    int ret = 0;
    pthread_rwlockattr_t attr;
    ret = pthread_rwlockattr_init(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_rwlockattr_init error",
                                               ret);
    }
    ret = pthread_rwlockattr_setpshared(&attr, sync_level_map.at(sl));
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_rwlockattr_setpshared error", ret);
    }
    ret = pthread_rwlock_init(&lock_, &attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_rwlock_init error",
                                               ret);
    }
    ret = pthread_rwlockattr_destroy(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_rwlockattr_destroy error", ret);
    }
}

rwlock::~rwlock() noexcept
{
    pthread_rwlock_destroy(&lock_);
}

void rwlock::unlock()
{
    int ret = pthread_rwlock_unlock(&lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_rwlock_unlock error",
                                               ret);
    }
}

void rwlock::rdlock()
{
    int ret = pthread_rwlock_rdlock(&lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_rwlock_rdlock error",
                                               ret);
    }
}

void rwlock::wrlock()
{
    int ret = pthread_rwlock_wrlock(&lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_rwlock_wrlock error",
                                               ret);
    }
}

bool rwlock::try_rdlock()
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
    throw_system_error_with_specific_errno("pthread_rwlock_tryrdlock error",
                                           ret);
    return ret;
}

bool rwlock::try_wrlock()
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
    throw_system_error_with_specific_errno("pthread_rwlock_trywrlock error",
                                           ret);
    return ret;
}

rdlockguard::rdlockguard(rwlock &lock) : rwlock_(&lock)
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
        catch (...)
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

wrlockguard::wrlockguard(rwlock &lock) : rwlock_(&lock)
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
        catch (...)
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

}  // namespace cppev
