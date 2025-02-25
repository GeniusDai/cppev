#include "cppev/lock.h"

namespace cppev
{

spinlock::spinlock()
{
#ifdef CPPEV_SPINLOCK_USE_PTHREAD
    int ret = pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_spin_init error", ret);
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
        throw_system_error_with_specific_errno("pthread_spin_lock error", ret);
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
        throw_system_error_with_specific_errno("pthread_spin_unlock error", ret);
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
        throw_system_error_with_specific_errno("pthread_spin_trylock error", ret);
    }
    return true;
#else
    return !lock_.test_and_set(std::memory_order_acq_rel);
#endif
}

}   // namespace cppev
