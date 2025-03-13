#ifndef _cppev_utils_h_6C0224787A17_
#define _cppev_utils_h_6C0224787A17_

#include <cassert>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <ctime>
#include <functional>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "cppev/common.h"

namespace cppev
{

enum class CPPEV_PUBLIC priority
{
    highest = 100,  // Internally reserved, please DONOT use!
    p0 = 20,
    p1 = 19,
    p2 = 18,
    p3 = 17,
    p4 = 16,
    p5 = 15,
    p6 = 14,
    lowest = 1,  // Internally reserved, please DONOT use!
};

// clang-format off

struct CPPEV_PRIVATE enum_hash
{
    template <typename T> std::size_t operator()(const T &t)
        const noexcept
    {
        return std::hash<int>()(static_cast<int>(t));
    }
};

template <typename T, size_t I>
struct CPPEV_PRIVATE tuple_less
{
    bool operator()(const T &lhs, const T &rhs) const noexcept
    {
        return std::get<I>(lhs) < std::get<I>(rhs);
    }
};

template <typename T, size_t I>
struct CPPEV_PRIVATE tuple_greater
{
    bool operator()(const T &lhs, const T &rhs) const noexcept
    {
        return std::get<I>(lhs) > std::get<I>(rhs);
    }
};

// clang-format on

/*
 * Chrono
 */
CPPEV_PRIVATE std::string timestamp(time_t t = -1,
                                    const char *format = nullptr);

template <typename Clock = std::chrono::system_clock>
CPPEV_PRIVATE void sleep_until(const std::chrono::nanoseconds &stamp)
{
    std::this_thread::sleep_until(
        std::chrono::duration_cast<Clock::duration>(stamp));
}

template <typename Clock = std::chrono::system_clock>
CPPEV_PRIVATE typename Clock::time_point ceil_time_point(
    const typename Clock::time_point &point)
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
CPPEV_PUBLIC int64_t least_common_multiple(int64_t p, int64_t r);

CPPEV_PUBLIC int64_t least_common_multiple(const std::vector<int64_t> &nums);

CPPEV_PUBLIC int64_t greatest_common_divisor(int64_t p, int64_t r);

CPPEV_PUBLIC int64_t greatest_common_divisor(const std::vector<int64_t> &nums);

/*
 * Exception handling
 */
using errno_type = std::remove_reference<decltype(errno)>::type;

// Template function with only one parameter shall be placed former!!!

template <typename T>
CPPEV_PRIVATE std::ostringstream oss_writer(T err_code)
{
    std::ostringstream oss;
    oss << " : errno " << err_code << " ";
    return oss;
}

template <typename Prev, typename... Args>
CPPEV_PRIVATE std::ostringstream oss_writer(Prev prev, Args... args)
{
    std::ostringstream oss;
    oss << prev;
    oss << oss_writer(args...).str();
    return oss;
}

template <typename T>
CPPEV_PRIVATE errno_type errno_getter(T err_code)
{
    return err_code;
}

template <typename Prev, typename... Args>
CPPEV_PRIVATE errno_type errno_getter(Prev, Args... args)
{
    return errno_getter(args...);
}

template <typename... Args>
CPPEV_PUBLIC void throw_system_error_with_specific_errno(Args... args)
{
    std::ostringstream oss = oss_writer(args...);
    errno_type err_code = errno_getter(args...);
    throw std::system_error(std::error_code(err_code, std::system_category()),
                            oss.str());
}

template <typename... Args>
CPPEV_PUBLIC void throw_system_error(Args... args)
{
    throw_system_error_with_specific_errno(args..., errno);
}

template <typename... Args>
CPPEV_PUBLIC void throw_logic_error(Args... args)
{
    std::ostringstream oss;
    (oss << ... << args);
    throw std::logic_error(oss.str());
}

template <typename... Args>
CPPEV_PUBLIC void throw_runtime_error(Args... args)
{
    std::ostringstream oss;
    (oss << ... << args);
    throw std::runtime_error(oss.str());
}

/*
 * Process level signal handling
 */
CPPEV_PUBLIC void ignore_signal(int sig);

CPPEV_PUBLIC void reset_signal(int sig);

CPPEV_PUBLIC void handle_signal(
    int sig, sig_t handler =
                 [](int)
             {
             });

CPPEV_PUBLIC void send_signal(pid_t pid, int sig);

CPPEV_PUBLIC bool check_process(pid_t pid);

CPPEV_PUBLIC bool check_process_group(pid_t pgid);

/*
 * Thread level signal handling
 */
CPPEV_PUBLIC void thread_raise_signal(int sig);

CPPEV_PUBLIC void thread_block_signal(int sig);

CPPEV_PUBLIC void thread_block_signal(const std::vector<int> &sigs);

CPPEV_PUBLIC void thread_unblock_signal(int sig);

CPPEV_PUBLIC void thread_unblock_signal(const std::vector<int> &sigs);

CPPEV_PUBLIC void thread_suspend_for_signal(int sig);

CPPEV_PUBLIC void thread_suspend_for_signal(const std::vector<int> &sigs);

CPPEV_PUBLIC void thread_wait_for_signal(int sig);

CPPEV_PUBLIC int thread_wait_for_signal(const std::vector<int> &sigs);

CPPEV_PUBLIC bool thread_check_signal_mask(int sig);

CPPEV_PUBLIC bool thread_check_signal_pending(int sig);

/*
 * String operation
 */
CPPEV_PUBLIC std::string join(const std::vector<std::string> &str_arr,
                              const std::string &sep) noexcept;

CPPEV_PUBLIC std::string strip(const std::string &str,
                               const std::string &chars);

CPPEV_PUBLIC std::string lstrip(const std::string &str,
                                const std::string &chars);

CPPEV_PUBLIC std::string rstrip(const std::string &str,
                                const std::string &chars);

CPPEV_PUBLIC std::vector<std::string> split(const std::string &str,
                                            const std::string &sep);

}  // namespace cppev

#endif  // utils.h
