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
#define CPPEV_SPINLOCK_USE_PTHREAD
#endif

/*
    Usage of spinlock is usually not recommended.
    Only used when tasks with lock are really important and simple, and make sure you won't be scheduled
    out by os when holding the lock.
    Currently spinlock shared among process is not supported.
    By benchmark pthread implementation is about two times faster than atomic implementation.
 */
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
