#include <sstream>
#include <tuple>
#include <mutex>
#include <ctime>
#include <sstream>
#include <thread>
#include <string>
#include <unordered_set>
#include <cassert>
#include <unistd.h>
#include "cppev/utils.h"
#include "cppev/sysconfig.h"
#include "cppev/async_logger.h"

namespace cppev
{

async_logger::~async_logger()
{
    if (level_ < 0)
    {
        return;
    }
    stop_ = true;
    cond_.notify_one();
    join();
}

async_logger &async_logger::operator<<(const std::string &str)
{
    return (*this) << str.c_str();
}

async_logger &async_logger::operator<<(long x)
{
    return (*this) << std::to_string(x).c_str();
}

async_logger &async_logger::operator<<(int x)
{
    return (*this) << std::to_string(x).c_str();
}

async_logger &async_logger::operator<<(double x)
{
    return (*this) << std::to_string(x).c_str();
}

async_logger &async_logger::operator<<(float x)
{
    return (*this) << std::to_string(x).c_str();
}

void async_logger::write_header(buffer &buf)
{
    std::stringstream ss;
    tid_t thr_id = gettid();
    ss << "- [";
    if (level_ == 1)
    {
        ss << "INFO] [";
    }
    else
    {
        ss << "ERROR] [";
    }
    ss << timestamp();
#ifdef __linux__
    ss << "] [LWP " << thr_id << "] ";
#else
    ss << "] [TID 0x" << std::hex << thr_id << "] ";
#endif
    buf.put_string(ss.str());
}



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

namespace log
{

async_logger info(1);
async_logger error(2);
async_logger endl(-1);

}   // namespace log

}   // namespace cppev
