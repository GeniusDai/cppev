#ifndef _async_logger_buffered_h_6C0224787A17_
#define _async_logger_buffered_h_6C0224787A17_

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

    async_logger &operator<<(const long x);

    async_logger &operator<<(int x);

    async_logger &operator<<(const double x);

    async_logger &operator<<(const float x);

private:
    void write_debug(buffer *buf);

    std::recursive_mutex lock_;

    std::condition_variable_any cond_;

    buffer buffer_;

    int level_;

    bool stop_;

    int recur_level_;

    static std::mutex global_lock_;
};

}   // namespace cppev

#endif  // aysnc_logger_buffered.h