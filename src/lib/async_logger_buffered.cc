#include "cppev/async_logger.h"

#if !defined(__CPPEV_USE_HASHED_LOGGER__)

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
#include "cppev/utils.h"
#include "cppev/sysconfig.h"

namespace cppev
{

async_logger::async_logger(int level)
: level_(level), stop_(false), recur_level_(0)
{
    if (level_ < 0)
    {
        return;
    }
    run();
}

async_logger &async_logger::operator<<(const char *str)
{
    lock_.lock();
    if (0 == recur_level_++)
    {
        write_header(buffer_);
    }
    buffer_.put_string(str);
    return *this;
}

async_logger &async_logger::operator<<(const async_logger &)
{
    (*this) << "\n";
    auto recur = recur_level_;
    recur_level_ = 0;
    while (recur--)
    {
        lock_.unlock();
    }
    cond_.notify_one();
    return *this;
}

void async_logger::run_impl()
{
    while (!stop_)
    {
        std::unique_lock<std::recursive_mutex> lock(lock_);
        if (0 == buffer_.size())
        {
            cond_.wait(lock,
                [this]() -> bool
                {
                    return this->buffer_.size()||this->stop_;
                }
            );
        }
        if (buffer_.size())
        {
            write(level_, buffer_.rawbuf(), buffer_.size());
            buffer_.clear();
        }
    }
}

}// namespace cppev

#endif  // !defined(__CPPEV_USE_HASHED_LOGGER__)
