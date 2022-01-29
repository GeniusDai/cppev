#include "cppev/lock.h"

#if !defined(__CPPEV_USE_POSIX_RWLOCK__)

#include <pthread.h>
#include <unordered_set>
#include <cassert>

namespace cppev {

rwlock::rwlock() : count_(0), rd_waiters_(0), wr_waiters_(0) {
    if (pthread_mutex_init(&lock_, nullptr) != 0)
    { throw_system_error("pthread_mutex_init error"); }
    if (pthread_cond_init(&rd_cond_, nullptr) != 0)
    { throw_system_error("pthread_cond_init error"); }
    if (pthread_cond_init(&wr_cond_, nullptr) != 0)
    { throw_system_error("pthread_cond_init error"); }
}

rwlock::~rwlock() {
    if (pthread_mutex_destroy(&lock_) != 0)
    { throw_system_error("pthread_mutex_destroy error"); }
    if (pthread_cond_destroy(&rd_cond_) != 0)
    { throw_system_error("pthread_cond_destroy error"); }
    if (pthread_cond_destroy(&wr_cond_) != 0)
    { throw_system_error("pthread_cond_destroy error"); }
}

void rwlock::unlock() {
    if (pthread_mutex_lock(&lock_) != 0)
    { throw_system_error("pthread_mutex_lock error"); }
    assert(count_ != 0);
    if (count_ == -1) {
        count_++;
        wr_owner_ = 0;
    } else {
        count_--;
        rd_owners_.erase(gettid());
    }
    if (count_ == 0) {
        if (wr_waiters_) {
            if (pthread_cond_signal(&wr_cond_) != 0)
            { throw_system_error("pthread_cond_signal error"); }
        } else if (rd_waiters_) {
            if (pthread_cond_broadcast(&rd_cond_) != 0)
            { throw_system_error("pthread_cond_broadcast error"); }
        }
    }
    if (pthread_mutex_unlock(&lock_) != 0)
    { throw_system_error("pthread_mutex_unlock error"); }
}

void rwlock::rdlock() {
    if (pthread_mutex_lock(&lock_) != 0)
    { throw_system_error("pthread_mutex_lock error"); }
    while (count_ < 0) {
        rd_waiters_++;
        if (pthread_cond_wait(&rd_cond_, &lock_) != 0)
        { throw_system_error("pthread_cond_wait error"); }
        rd_waiters_--;
    }
    count_++;
    assert(wr_owner_ == 0);
    rd_owners_.insert(gettid());
    // if (rd_waiters_) {
    //     if (pthread_cond_broadcast(&rd_cond_) != 0)
    //     { throw_system_error("pthread_cond_broadcast error"); }
    // }
    if (pthread_mutex_unlock(&lock_) != 0)
    { throw_system_error("pthread_mutex_unlock error"); }
}

void rwlock::wrlock() {
    if (pthread_mutex_lock(&lock_) != 0)
    { throw_system_error("pthread_mutex_lock error"); }
    while (count_ != 0) {
        wr_waiters_++;
        if (pthread_cond_wait(&wr_cond_, &lock_) != 0)
        { throw_system_error("pthread_cond_wait error"); }
        wr_waiters_--;
    }
    count_--;
    assert(wr_owner_ == 0);
    wr_owner_ = gettid();
    if (pthread_mutex_unlock(&lock_) != 0)
    { throw_system_error("pthread_mutex_unlock error"); }
}

}

#endif