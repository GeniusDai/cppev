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

private:
    void write_header(buffer &buf);

    int level_;

    bool stop_;

    std::recursive_mutex lock_;

    std::condition_variable_any cond_;

    int curr_;

    int recur_level_;

    std::vector<buffer> buffers_;
};

namespace log
{

extern async_logger info;
extern async_logger error;
extern async_logger endl;

}   // namespace log

}   // namespace cppev

#endif  // async_logger.h
