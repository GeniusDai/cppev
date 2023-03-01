#ifndef _utils_h_6C0224787A17_
#define _utils_h_6C0224787A17_

#include <functional>
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <ctime>
#include <csignal>
#include <cassert>

namespace cppev
{

enum priority
{
    low = 10,
    mid = 15,
    high = 20,
};

struct enum_hash
{
    template <typename T>
    std::size_t operator()(const T &t) const
    {
        return std::hash<int>()((int)t);
    }
};

/*
 * Chrono
 */
time_t time();

std::string timestamp(time_t t = -1, const char *format = nullptr);

template<typename Clock = std::chrono::system_clock>
void sleep_until(const std::chrono::nanoseconds &stamp)
{
    std::this_thread::sleep_until(std::chrono::duration_cast<Clock::duration>(stamp));
}

/*
 * Algorithm
 */
int64_t least_common_multiple(int64_t p, int64_t r);

int64_t least_common_multiple(const std::vector<int64_t> &nums);

int64_t greatest_common_divisor(int64_t p, int64_t r);

int64_t greatest_common_divisor(const std::vector<int64_t> &nums);

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
void thread_yield() noexcept;

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

tid_t gettid() noexcept;

/*
 * split : doesn't support string contains '\0'
 * join  : support string contains '\0'
 */
std::vector<std::string> split(const char *str, const char *sep) noexcept;

std::vector<std::string> split(const std::string &str, const std::string &sep) noexcept;

std::string join(const std::vector<std::string> &str_arr, const std::string &sep) noexcept;

}   // namespace utils

}   // namespace cppev

#endif  // utils.h