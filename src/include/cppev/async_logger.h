#ifndef _async_logger_h_6C0224787A17_
#define _async_logger_h_6C0224787A17_

#include <unordered_map>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "cppev/buffer.h"
#include "cppev/common_utils.h"
#include "cppev/runnable.h"
#include "cppev/lock.h"

namespace cppev {

class async_logger final : public runnable {
public:
    explicit async_logger(int level) : level_(level), stop_(false) {
        if (level_ < 0) { return; }
        run();
    }

    ~async_logger() {
        if (level_ < 0) { return; }
        stop_ = true;
        cond_.notify_one();
        join();
    }

    void run_impl() override;

    async_logger &operator<<(const std::string str);

    async_logger &operator<<(const int x);

    async_logger &operator<<(const async_logger &);

private:
    void write_debug();

    // thread_id --> < buffer, recursive_mutex, recursive_level, timestamp >
    std::unordered_map<tid, std::tuple<std::shared_ptr<buffer>,
        std::shared_ptr<std::recursive_mutex>, int, time_t> > logs_;

    int level_;

    std::condition_variable_any cond_;

    rwlock lock_;

    bool stop_;

    static std::mutex global_lock_;
};

namespace log
{
    extern async_logger info;
    extern async_logger error;
    extern async_logger endl;
}

}

#endif