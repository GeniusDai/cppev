#include "cppev/utils.h"
#include <string>
#include <cstring>
#include <exception>
#include <system_error>

#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <pthread.h>
#endif

namespace cppev
{

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
    throw std::runtime_error( str);
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
    if (pid == 0 || pid == -1)
    {
        throw_logic_error("check for 0 or -1 is always ok and not supported");
    }
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

/*
 * Q: Why not support waiting for a set of signals?
 * A: One thread waiting for one signal. If you need to wait for several signals, just
 *    create several threads.
 */
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

void thread_suspend_for_signal(int sig)
{
    sigset_t set;
    sigfillset(&set);
    sigdelset(&set, sig);
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

void thread_raise_signal(int sig)
{
    if (raise(sig) != 0)
    {
        throw_system_error("raise error");
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

namespace utils
{

tid gettid()
{
#ifdef __linux__
    static thread_local tid thr_id = syscall(SYS_gettid);
#elif defined(__APPLE__)
    static thread_local tid thr_id = 0;
    if (thr_id == 0)
    {
        pthread_threadid_np(0, &thr_id);
    }
#endif
    return thr_id;
}

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

std::vector<std::string> split(const std::string &str, const std::string &sep)
{
    return split(str.c_str(), sep.c_str());
}

std::vector<std::string> split(const char *str, const char *sep)
{
    size_t str_len = strlen(str);
    size_t sep_len = strlen(sep);

    if (sep_len > str_len)
    {
        return { str };
    }

    std::vector<int> sep_index;

    sep_index.push_back(0 - sep_len);

    for (size_t i = 0; i <= str_len - sep_len; )
    {
        bool is_sep = true;
        for (size_t j = i; j < i + sep_len; ++j)
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
            i += sep_len;
        }
        else
        {
            ++i;
        }
    }

    sep_index.push_back(str_len);

    if (sep_index.size() == 2)
    {
        return { str };
    }

    std::vector<std::string> substrs;

    for (size_t i = 1; i < sep_index.size(); ++i)
    {
        int begin = sep_index[i-1] + sep_len;
        int end = sep_index[i];

        substrs.push_back(std::string(str, begin, end - begin));
    }
    return substrs;
}

std::string join(const std::vector<std::string> &str_arr, const std::string &sep)
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

}   // namespace utils

}   // namespace cppev