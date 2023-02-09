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

/*
 * Exception handling
 */
void throw_logic_error(const std::string &str);

void throw_system_error(const std::string &str, int err=0);

void throw_runtime_error(const std::string &str);

/*
 * Process level signal handling
 */
void ignore_signal(int sig);

void reset_signal(int sig);

void handle_signal(int sig, sig_t handler=[](int){});

void send_signal(pid_t pid, int sig);

bool check_process(pid_t pid);

bool check_process_group(pid_t pgid);

/*
 * Thread level signal handling
 */
void thread_block_signal(int sig);

void thread_unblock_signal(int sig);

bool thread_check_signal_mask(int sig);

void thread_raise_signal(int sig);

void thread_suspend_for_signal(int sig);

void thread_wait_for_signal(int sig);

bool thread_check_signal_pending(int sig);

/*
 * Thread controlling
 */
void thread_yield();

void thread_cancel_point();

#ifdef __linux__
typedef pid_t tid_t;
#elif defined(__APPLE__)
typedef uint64_t tid_t;
#else
static_assert(false, "platform not supported");
#endif

namespace utils
{

tid_t gettid();

time_t time();

std::string timestamp(time_t t = -1, const char *format = nullptr);

/*
 * split : doesn't support string contains '\0'
 * join  : support string contains '\0'
 */
std::vector<std::string> split(const char *str, const char *sep);

std::vector<std::string> split(const std::string &str, const std::string &sep);

std::string join(const std::vector<std::string> &str_arr, const std::string &sep);

}   // namespace utils

}   // namespace cppev

#endif  // utils.h