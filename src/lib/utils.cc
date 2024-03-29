#include "cppev/utils.h"
#include <string>
#include <cstring>
#include <exception>
#include <system_error>
#include <sched.h>

#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <pthread.h>
#endif

namespace cppev
{

time_t time()
{
    time_t t = ::time(nullptr);
    if (t == -1)
    {
        throw_system_error("time error");
    }
    return t;
}

std::string timestamp(time_t t, const char *format)
{
    static_assert(std::is_signed<time_t>::value, "time_t is not signed!");
    if (t < 0)
    {
        t = time();
    }
    if (format == nullptr)
    {
        format = "%F %T %Z";
    }
    tm s_tm;
    localtime_r(&t, &s_tm);
    char buf[1024];
    memset(buf, 0, 1024);
    if (0 == strftime(buf, 1024, format, &s_tm))
    {
        throw_system_error("strftime error");
    }
    return buf;
}

int64_t least_common_multiple(int64_t p, int64_t r)
{
    assert(p != 0 && r != 0);
    return (p / greatest_common_divisor(p ,r)) * r;
}

int64_t least_common_multiple(const std::vector<int64_t> &nums)
{
    assert(nums.size() >= 2);
    int64_t prev = nums[0];
    for (size_t i = 1; i < nums.size(); ++i)
    {
        prev = least_common_multiple(prev, nums[i]);
    }
    return prev;
}

int64_t greatest_common_divisor(int64_t p, int64_t r)
{
    assert(p != 0 && r != 0);
    int64_t remain;
    while (true)
    {
        remain = p % r;
        p = r;
        r = remain;
        if (remain == 0)
        {
            break;
        }
    }
    return p;
}

int64_t greatest_common_divisor(const std::vector<int64_t> &nums)
{
    assert(nums.size() >= 2);
    int64_t prev = nums[0];
    for (size_t i = 1; i < nums.size(); ++i)
    {
        prev = greatest_common_divisor(prev, nums[i]);
    }
    return prev;
}

void throw_system_error(const std::string &str, int err)
{
    if (err == 0)
    {
        err = errno;
    }
    throw std::system_error(std::error_code(err, std::system_category()),
        std::string(str).append(" : errno ").append(std::to_string(err)).append(" "));
}

void throw_logic_error(const std::string &str)
{
    throw std::logic_error(str);
}

void throw_runtime_error(const std::string &str)
{
    throw std::runtime_error(str);
}

void ignore_signal(int sig)
{
    handle_signal(sig, SIG_IGN);
}

void reset_signal(int sig)
{
    handle_signal(sig, SIG_DFL);
}

void handle_signal(int sig, sig_t handler)
{
    struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = handler;
    if (sigaction(sig, &sigact, nullptr) == -1)
    {
        throw_system_error("sigaction error");
    }
}

void send_signal(pid_t pid, int sig)
{
    if (sig == 0)
    {
        throw_logic_error("pid or pgid check is not supported");
    }
    if (kill(pid, sig) != 0)
    {
        throw_system_error("kill error");
    }
}

bool check_process(pid_t pid)
{
    if (kill(pid, 0) == 0)
    {
        return true;
    }
    else
    {
        if (errno == ESRCH)
        {
            return false;
        }
        throw_system_error("kill error");
    }
    return true;
}

bool check_process_group(pid_t pgid)
{
    return check_process(-1 * pgid);
}



void thread_raise_signal(int sig)
{
    if (raise(sig) != 0)
    {
        throw_system_error("raise error");
    }
}

void thread_wait_for_signal(int sig)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);

    // linux requires non-null
    int ret_sig;
    if (sigwait(&set, &ret_sig) != 0)
    {
        throw_system_error("sigwait error");
    }
}

int thread_wait_for_signal(const std::vector<int> &sigs)
{
    sigset_t set;
    sigemptyset(&set);
    for (auto sig : sigs)
    {
        sigaddset(&set, sig);
    }

    // linux requires non-null
    int ret_sig;
    if (sigwait(&set, &ret_sig) != 0)
    {
        throw_system_error("sigwait error");
    }
    return ret_sig;
}

void thread_suspend_for_signal(int sig)
{
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, sig);
    // sigsuspend always returns -1
    sigsuspend(&set);
}

void thread_suspend_for_signal(const std::vector<int> &sigs)
{
    sigset_t set;
    sigfillset(&set);
    for (auto sig : sigs)
    {
        sigdelset(&set, sig);
    }
    // sigsuspend always returns -1
    sigsuspend(&set);
}

void thread_block_signal(int sig)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);
    int ret = pthread_sigmask(SIG_BLOCK, &set, nullptr);
    if (ret != 0)
    {
        throw_system_error("pthread_sigmask error", ret);
    }
}

void thread_block_signal(const std::vector<int> &sigs)
{
    sigset_t set;
    sigemptyset(&set);
    for (auto sig : sigs)
    {
        sigaddset(&set, sig);
    }
    int ret = pthread_sigmask(SIG_BLOCK, &set, nullptr);
    if (ret != 0)
    {
        throw_system_error("pthread_sigmask error", ret);
    }
}

void thread_unblock_signal(int sig)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);
    int ret = pthread_sigmask(SIG_UNBLOCK, &set, nullptr);
    if (ret != 0)
    {
        throw_system_error("pthread_sigmask error", ret);
    }
}

void thread_unblock_signal(const std::vector<int> &sigs)
{
    sigset_t set;
    sigemptyset(&set);
    for (auto sig : sigs)
    {
        sigaddset(&set, sig);
    }
    int ret = pthread_sigmask(SIG_UNBLOCK, &set, nullptr);
    if (ret != 0)
    {
        throw_system_error("pthread_sigmask error", ret);
    }
}

bool thread_check_signal_mask(int sig)
{
    sigset_t set;
    sigemptyset(&set);
    int ret = pthread_sigmask(SIG_SETMASK, nullptr, &set);
    if (ret != 0)
    {
        throw_system_error("pthread_sigmask error", ret);
    }
    return sigismember(&set, sig) == 1;
}

bool thread_check_signal_pending(int sig)
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);
    if (sigpending(&set) != 0)
    {
        throw_system_error("sigpending error");
    }
    return sigismember(&set, sig) == 1;
}



tid_t gettid() noexcept
{
    thread_local tid_t thr_id = 0;
    if (thr_id == 0)
    {
#ifdef __linux__
        thr_id = syscall(SYS_gettid);
#elif defined(__APPLE__)
        pthread_threadid_np(0, &thr_id);
#endif  // __linux__
    }
    return thr_id;
}

std::vector<std::string> split(const std::string &str, const std::string &sep)
{
    if (sep.empty())
    {
        throw_runtime_error("cannot split string with empty seperator");
    }

    if (sep.size() > str.size())
    {
        return { str };
    }

    std::vector<int> sep_index;

    sep_index.push_back(0 - sep.size());

    for (size_t i = 0; i <= str.size() - sep.size(); )
    {
        bool is_sep = true;
        for (size_t j = i; j < i + sep.size(); ++j)
        {
            if (str[j] != sep[j-i])
            {
                is_sep = false;
                break;
            }
        }
        if (is_sep)
        {
            sep_index.push_back(static_cast<int>(i));
            i += sep.size();
        }
        else
        {
            ++i;
        }
    }

    sep_index.push_back(str.size());

    if (sep_index.size() == 2)
    {
        return { str };
    }

    std::vector<std::string> substrs;

    for (size_t i = 1; i < sep_index.size(); ++i)
    {
        int begin = sep_index[i-1] + sep.size();
        int end = sep_index[i];

        substrs.push_back(std::string(str, begin, end - begin));
    }
    return substrs;
}

std::string join(const std::vector<std::string> &str_arr, const std::string &sep) noexcept
{
    std::string ret = "";
    size_t size = 0;
    for (size_t i = 0; i < str_arr.size(); ++i)
    {
        size += str_arr[i].size();
    }
    size += sep.size() * (str_arr.size() - 1);
    ret.reserve(size);
    for (size_t i = 0; i < str_arr.size(); ++i)
    {
        ret += str_arr[i];
        if (i != str_arr.size() - 1)
        {
            ret += sep;
        }
    }
    return ret;
}

static constexpr int STRIP_LEFT = 0x01;
static constexpr int STRIP_RIGHT = 0x10;

static std::string do_strip(const std::string &str, const std::string &chars, const int type)
{
    if (chars.empty())
    {
        throw_runtime_error("cannot strip string with empty strip chars");
    }

    size_t p = 0;
    size_t r = str.size();

    if (type & STRIP_LEFT)
    {
        while (r - p >= chars.size())
        {
            bool eq = true;
            size_t j = 0;
            for (size_t i = p; i < p + chars.size(); ++i)
            {
                if (str[i] != chars[j++])
                {
                    eq = false;
                    break;
                }
            }
            if (eq)
            {
                p += chars.size();
            }
            else
            {
                break;
            }
        }
    }

    if (type & STRIP_RIGHT)
    {
        while (r - p >= chars.size())
        {
            bool eq = true;
            size_t j = chars.size() - 1;
            for (size_t i = r - 1; i > r - chars.size() - 1; --i)
            {
                if (str[i] != chars[j--])
                {
                    eq = false;
                    break;
                }
            }
            if (eq)
            {
                r -= chars.size();
            }
            else
            {
                break;
            }
        }
    }

    return str.substr(p ,r - p);
}

std::string strip(const std::string &str, const std::string &chars)
{
    return do_strip(str, chars, STRIP_LEFT | STRIP_RIGHT);
}

std::string lstrip(const std::string &str, const std::string &chars)
{
    return do_strip(str, chars, STRIP_LEFT);
}

std::string rstrip(const std::string &str, const std::string &chars)
{
    return do_strip(str, chars, STRIP_RIGHT);
}


}   // namespace cppev