#ifndef _cppev_lock_h_6C0224787A17_
#define _cppev_lock_h_6C0224787A17_

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

}   // namespace cppev

#endif  // lock.h
