#ifndef _common_utils_h_6C0224787A17_
#define _common_utils_h_6C0224787A17_

#include <functional>
#include <ctime>
#include <string>
#include <thread>
#include <vector>

namespace cppev
{

struct enum_hash
{
    template <typename T>
    std::size_t operator()(const T &t) const
    {
        return std::hash<int>()((int)t);
    }
};

class uncopyable
{
public:
    uncopyable() {}
    virtual ~uncopyable() noexcept {}
private:
    uncopyable &operator=(const uncopyable &) = delete;
    uncopyable(const uncopyable&) = delete;
};

void throw_logic_error(const char *str);

void throw_system_error(const char *str);

void throw_runtime_error(const char *str);

#ifdef __linux__
typedef pid_t tid;
#elif defined(__APPLE__)
typedef uint64_t tid;
#endif

namespace utils
{

tid gettid();

void ignore_signal(int sig);

time_t time();

std::string timestamp(time_t t = 0, const char *format = nullptr);

std::vector<std::string> split(const char *str, const char *sep);

std::vector<std::string> split(const std::string &str, const std::string &sep);

}   // namespace utils

}   // namespace cppev

#endif  // common_utils.h