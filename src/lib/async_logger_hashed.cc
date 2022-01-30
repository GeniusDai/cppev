#include "cppev/async_logger.h"

#if defined(__CPPEV_USE_HASHED_LOGGER__)

#include <tuple>
#include <mutex>
#include <ctime>
#include <sstream>
#include <thread>
#include <string>
#include <unordered_set>
#include <unistd.h>
#include <cassert>
#include <ios>
#include "cppev/lock.h"
#include "cppev/common_utils.h"
#include "cppev/sysconfig.h"

namespace cppev {

async_logger::async_logger(int level)
: level_(level), stop_(false)
{
    if (level_ < 0) { return; }
    run();
}

async_logger &async_logger::operator<<(const char *str) {
    tid thr_id = gettid();
    {
        rdlockguard rdlock(lock_);
        if (logs_.count(thr_id) != 0) {
            std::get<1>(logs_[thr_id])->lock();
            if (++std::get<2>(logs_[thr_id]) == 1)
            { write_debug(std::get<0>(logs_[thr_id]).get()); }
            std::get<0>(logs_[thr_id])->put(str);
            return *this;
        }
    }
    wrlockguard wrlock(lock_);
    logs_[thr_id] = std::make_tuple<>(std::shared_ptr<buffer>(new buffer),
        std::shared_ptr<std::recursive_mutex>(new std::recursive_mutex()), 1, sc_time());
    write_debug(std::get<0>(logs_[thr_id]).get());
    std::get<1>(logs_[thr_id])->lock();
    std::get<0>(logs_[thr_id])->put(str);
    return *this;
}

async_logger &async_logger::operator<<(const async_logger &) {
    (*this) << "\n";
    tid thr_id = gettid();
    {
        rdlockguard rdlock(lock_);
        int lock_count = std::get<2>(logs_[thr_id]);
        std::get<2>(logs_[thr_id]) = 0;
        std::get<3>(logs_[thr_id]) = sc_time();
        while(lock_count--) { std::get<1>(logs_[thr_id])->unlock(); }
    }
    cond_.notify_one();
    return *this;
}

void async_logger::run_impl() {
    while (true) {
        std::unordered_set<tid> outdate_list;
        bool delay = true;
        {
            rdlockguard rdlock(lock_);
            for (auto iter = logs_.begin(); iter != logs_.end(); ++iter) {
                std::unique_lock<std::recursive_mutex>
                    lock(*(std::get<1>(iter->second).get()));
                if (std::get<0>(iter->second)->size()) {
                    std::unique_lock<std::mutex> glock(global_lock_);
                    buffer *buf = std::get<0>(iter->second).get();
                    write(level_, buf->buf(), buf->size());
                    buf->clear();
                    delay = false;
                } else {
                    if (sc_time() - std::get<3>(iter->second) > sysconfig::buffer_outdate)
                    { outdate_list.insert(iter->first); }
                }
            }
        }
        if (outdate_list.size() || delay) {
            wrlockguard wrlock(lock_);
            for (auto iter = logs_.begin(); iter != logs_.end(); ) {
                if (0 != std::get<0>(iter->second)->size()) {
                    if (stop_) {
                        assert(0 == std::get<2>(iter->second));
                        std::unique_lock<std::mutex> glock(global_lock_);
                        buffer *buf = std::get<0>(iter->second).get();
                        write(level_, buf->buf(), buf->size());
                        buf->clear();
                    }
                    delay = false;
                    ++iter;
                } else {
                    assert(0 == std::get<2>(iter->second));
                    if (outdate_list.count(iter->first)) { iter = logs_.erase(iter); }
                    else { ++iter; }
                }
            }
            if (stop_) { break; }
            if (delay) { cond_.wait(wrlock); }
        }
    }
}

}

#endif  // __CPPEV_USE_HASHED_LOGGER__
