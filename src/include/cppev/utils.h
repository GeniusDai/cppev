#ifndef _utils_h_6C0224787A17_
#define _utils_h_6C0224787A17_

#include <functional>
#include <ctime>
#include <string>
#include <thread>
#include <vector>
#include <csignal>

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

void throw_logic_error(const std::string &str);

void throw_system_error(const std::string &str, int err=0);

void throw_runtime_error(const std::string &str);

/*
 * Process level
 */
void ignore_signal(int sig);

void reset_signal(int sig);

void handle_signal(int sig, sig_t handler=[](int){});

void send_signal(pid_t pid, int sig);

/*
 * Thread level
 */
void thread_block_signal(int sig);

void thread_unblock_signal(int sig);

bool thread_check_signal_mask(int sig);

void thread_raise_signal(int sig);

void thread_wait_for_signal(int sig);

bool thread_check_signal_pending(int sig);

#ifdef __linux__
typedef pid_t tid;
#elif defined(__APPLE__)
typedef uint64_t tid;
#else
static_assert(false, "platform not supported");
#endif

namespace utils
{

tid gettid();

time_t time();

std::string timestamp(time_t t = 0, const char *format = nullptr);

std::vector<std::string> split(const char *str, const char *sep);

std::vector<std::string> split(const std::string &str, const std::string &sep);

}   // namespace utils

}   // namespace cppev

#endif  // utils.h