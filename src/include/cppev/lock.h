#ifndef _lock_h_6C0224787A17_
#define _lock_h_6C0224787A17_

#include "cppev/common_utils.h"
#include <pthread.h>
#include <exception>
#include <unordered_set>

namespace cppev {

// spinlock for linux
#if _POSIX_C_SOURCE >= 200112L

class spinlock final : public uncopyable {
public:
    spinlock() {
        if (pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE) != 0)
        { throw_system_error("pthread_spin_init error"); }
    }

    ~spinlock() {
        if (pthread_spin_destroy(&lock_) != 0)
        { throw_system_error("pthread_spin_destroy error"); }
    }

    void lock() {
        if (pthread_spin_lock(&lock_) != 0)
        { throw_system_error("pthread_spin_lock error"); }
    }

    void unlock() {
        if (pthread_spin_unlock(&lock_) != 0)
        { throw_system_error("pthread_spin_unlock error"); }
    }

    bool trylock() {
        if (pthread_spin_trylock(&lock_) == 0) { return true; }
        if (errno == EBUSY) { return false; }
        throw_system_error("pthread_spin_trylock error");
    }

private:
    pthread_spinlock_t lock_;
};

#endif
#define __CPPEV_USE_POSIX_RWLOCK__
#if defined(__CPPEV_USE_POSIX_RWLOCK__)

class rwlock final : public uncopyable {
public:
    rwlock() {
        if (pthread_rwlock_init(&lock_, nullptr) != 0)
        { throw_system_error("pthread_rwlock_init error"); }
    }

    ~rwlock() {
        if (pthread_rwlock_destroy(&lock_) != 0)
        { throw_system_error("pthread_rwlock_destroy error"); }
    }

    void unlock() {
        if (::pthread_rwlock_unlock(&lock_) != 0)
        { throw_system_error("pthread_rwlock_destroy error"); }
    }

    void rdlock() {
        if (::pthread_rwlock_rdlock(&lock_) != 0)
        { throw_system_error("pthread_rwlock_rdlock error"); }

    }
    void wrlock() {
        if (::pthread_rwlock_wrlock(&lock_) != 0)
        { throw_system_error("pthread_rwlock_wrlock error"); }

    }

private:
    pthread_rwlock_t lock_;
};

#else

class rwlock final : public uncopyable {
public:
    rwlock();

    ~rwlock();

    void unlock();

    void rdlock();

    void wrlock();

private:
    // mutex protect: count_ / rd_waiters / wr_waiters / wr_owner / rd_owners
    pthread_mutex_t lock_;

    // cv for threads waiting for read lock
    pthread_cond_t rd_cond_;

    // cv for threads waiting for write lock 
    pthread_cond_t wr_cond_;

    // reference count value
    int count_;

    // number of threads waiting for read lock
    int rd_waiters_;

    // number of threads waiting for write lock
    int wr_waiters_;

    // for debug: write lock owner
    tid wr_owner_;

    // for debug: read lock owners
    std::unordered_set<tid> rd_owners_;
};

#endif

class rdlockguard final : public uncopyable {
public:
    rdlockguard(rwlock &lock) { rwlock_ = &lock; rwlock_->rdlock(); }

    ~rdlockguard() { rwlock_->unlock(); }

    void lock() { rwlock_->rdlock(); }

    void unlock() { rwlock_->unlock(); }

private:
    rwlock *rwlock_;
};

class wrlockguard final : public uncopyable {
public:
    wrlockguard(rwlock &lock) { rwlock_ = &lock; rwlock_->wrlock(); }

    ~wrlockguard() { rwlock_->unlock(); }

    void lock() { rwlock_->wrlock(); }

    void unlock() { rwlock_->unlock(); }

private:
    rwlock *rwlock_;
};

}

#endif