#include "cppev/lock.h"

#if !defined(__CPPEV_USE_POSIX_RWLOCK__)

#include <pthread.h>
#include <unordered_set>
#include <cassert>

namespace cppev {

void rwlock::unlock() {
    std::unique_lock<std::mutex> lock(lock_);
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
            wr_cond_.notify_one();
        } else if (rd_waiters_) {
            rd_cond_.notify_all();
        }
    }
}

void rwlock::rdlock() {
    std::unique_lock<std::mutex> lock(lock_);
    while (count_ < 0) {
        rd_waiters_++;
        rd_cond_.wait(lock);
        rd_waiters_--;
    }
    count_++;
    assert(wr_owner_ == 0);
    rd_owners_.insert(gettid());
}

void rwlock::wrlock() {
    std::unique_lock<std::mutex> lock(lock_);
    while (count_ != 0) {
        wr_waiters_++;
        wr_cond_.wait(lock);
        wr_waiters_--;
    }
    count_--;
    assert(wr_owner_ == 0 && rd_owners_.empty());
    wr_owner_ = gettid();
}

}

#endif