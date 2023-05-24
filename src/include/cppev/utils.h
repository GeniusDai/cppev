#ifndef _utils_h_6C0224787A17_
#define _utils_h_6C0224787A17_

#include <cstdint>
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
    p0 = 20,
    p1 = 19,
    p2 = 18,
    p3 = 17,
    p4 = 16,
    p5 = 15,
    p6 = 14,
};

struct enum_hash
{
    template <typename T>
    std::size_t operator()(const T &t) const noexcept
    {
        return std::hash<int>()(static_cast<int>(t));
    }
};

template <typename T, size_t I>
struct tuple_less
{
    bool operator()(const T& lhs, const T &rhs) const noexcept
    {
        return std::get<I>(lhs) < std::get<I>(rhs);
    }
};

template <typename T, size_t I>
struct tuple_greater
{
    bool operator()(const T& lhs, const T &rhs) const noexcept
    {
        return std::get<I>(lhs) > std::get<I>(rhs);
    }
};

/*
 * Chrono
 */
time_t time();

std::string timestamp(time_t t = -1, const char *format = nullptr);

template <typename Clock = std::chrono::system_clock>
void sleep_until(const std::chrono::nanoseconds &stamp)
{
    std::this_thread::sleep_until(std::chrono::duration_cast<Clock::duration>(stamp));
}

template <typename Clock = std::chrono::system_clock>
typename Clock::time_point ceil_time_point(const typename Clock::time_point &point)
{
    auto stamp = std::chrono::nanoseconds(point.time_since_epoch()).count();
    int64_t ceil_stamp_nsec = (stamp / 1'000'000'000 + 1) * 1'000'000'000;
    auto ceil_stamp = std::chrono::duration_cast<typename Clock::duration>(
        std::chrono::nanoseconds(ceil_stamp_nsec));
    return typename Clock::time_point(ceil_stamp);
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
void thread_raise_signal(int sig);

void thread_block_signal(int sig);

void thread_block_signal(const std::vector<int> &sigs);

void thread_unblock_signal(int sig);

void thread_unblock_signal(const std::vector<int> &sigs);

void thread_suspend_for_signal(int sig);

void thread_suspend_for_signal(const std::vector<int> &sigs);

void thread_wait_for_signal(int sig);

int thread_wait_for_signal(const std::vector<int> &sigs);

bool thread_check_signal_mask(int sig);

bool thread_check_signal_pending(int sig);

#ifdef __linux__
typedef pid_t tid_t;
#elif defined(__APPLE__)
typedef uint64_t tid_t;
#else
static_assert(false, "platform not supported");
#endif

tid_t gettid() noexcept;

std::string join(const std::vector<std::string> &str_arr, const std::string &sep) noexcept;

std::string strip(const std::string &str, const std::string &chars);

std::string lstrip(const std::string &str, const std::string &chars);

std::string rstrip(const std::string &str, const std::string &chars);

std::vector<std::string> split(const std::string &str, const std::string &sep);

}   // namespace cppev

#endif  // utils.h