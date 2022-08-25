#ifndef _async_logger_hashed_h_6C0224787A17_
#define _async_logger_hashed_h_6C0224787A17_

#include <unordered_map>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "cppev/buffer.h"
#include "cppev/common_utils.h"
#include "cppev/runnable.h"
#include "cppev/lock.h"

namespace cppev
{

class async_logger final
: public runnable
{
public:
    explicit async_logger(int level);

    ~async_logger();

    void run_impl() override;

    async_logger &operator<<(const char *str);

    async_logger &operator<<(const async_logger &);

    async_logger &operator<<(const std::string &str);

    async_logger &operator<<(long x);

    async_logger &operator<<(int x);

    async_logger &operator<<(double x);

    async_logger &operator<<(float x);

    static std::string version();

private:
    void write_debug(buffer *buf);

    // thread_id --> < buffer, recursive_mutex, recursive_level, utils::timestamp >
    std::unordered_map<tid, std::tuple<std::shared_ptr<buffer>,
        std::shared_ptr<std::recursive_mutex>, int, time_t> > logs_;

    std::condition_variable_any cond_;

    int level_;

    pshared_rwlock lock_;

    bool stop_;

    static std::mutex global_lock_;
};

}   // namespace cppev

#endif  // aysnc_logger_hashed.h