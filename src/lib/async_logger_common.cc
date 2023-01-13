#include "cppev/async_logger.h"
#include <sstream>

// Common function for both implementation

namespace cppev
{

std::mutex async_logger::global_lock_;

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

void async_logger::write_debug(buffer *buf)
{
    std::stringstream ss;
    tid thr_id = utils::gettid();
    ss << "- [";
    if (level_ == 1)
    {
        ss << "INFO] [";
    }
    else
    {
        ss << "ERROR] [";
    }
    ss << utils::timestamp();
#ifdef __linux__
    ss << "] [LWP " << thr_id << "] ";
#elif defined(__APPLE__)
    ss << "] [TID 0x" << std::hex << thr_id << "] ";
#endif
    buf->put_string(ss.str());
}

std::string async_logger::version()
{
#if defined(__CPPEV_USE_HASHED_LOGGER__)
    return "hashed";
#else
    return "buffered";
#endif
}

namespace log
{

async_logger info(1);
async_logger error(2);
async_logger endl(-1);

}   // namespace log

}   // namespace cppev
