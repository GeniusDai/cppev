#ifndef _async_logger_h_6C0224787A17_
#define _async_logger_h_6C0224787A17_

#include <unordered_map>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <shared_mutex>
#include <memory>
#include "cppev/buffer.h"
#include "cppev/utils.h"
#include "cppev/runnable.h"

// Only use hashed async logger in linux with higher version
// glibc, lower version glibc or macOS got bug in read-write-lock
#if defined(__linux__) && defined(__GNUC_PREREQ)
#if __GNUC_PREREQ(2, 25)
#define __CPPEV_USE_HASHED_LOGGER__
#endif
#endif

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

    static constexpr const char *impl()
    {
#if defined(__CPPEV_USE_HASHED_LOGGER__)
        return "hashed";
#else
        return "buffered";
#endif
    }

private:
    void write_header(buffer &buf);

    int level_;

    bool stop_;

#if defined(__CPPEV_USE_HASHED_LOGGER__)

    // thread_id --> < buffer, recursive_mutex, recursive_level, timestamp >
    std::unordered_map<tid_t, std::tuple<buffer,
        std::unique_ptr<std::recursive_mutex>, int, time_t>> logs_;

    std::shared_mutex lock_;

#else

    std::recursive_mutex lock_;

    buffer buffer_;

    int recur_level_;

#endif  // __CPPEV_USE_HASHED_LOGGER__

    std::condition_variable_any cond_;
};

namespace log
{

extern async_logger info;
extern async_logger error;
extern async_logger endl;

}   // namespace log

}   // namespace cppev

#endif  // async_logger.h
