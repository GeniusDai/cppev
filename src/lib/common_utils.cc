#include "cppev/common_utils.h"
#include <string>
#include <cstring>
#include <exception>
#include <system_error>
#include <signal.h>

#ifdef __linux__
#include <sys/syscall.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include <pthread.h>
#endif

namespace cppev
{

void throw_system_error(const char *str)
{
    throw std::system_error(std::error_code(errno, std::system_category()) ,str);
}

void throw_logic_error(const char *str)
{
    throw std::logic_error(str);
}

void throw_runtime_error(const char *str)
{
    throw std::runtime_error( str);
}

namespace utils {

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

void ignore_signal(int sig)
{
    struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = SIG_IGN;
    if (sigaction(sig, &sigact, nullptr) == -1)
    {
        throw_system_error("sigaction error");
    }
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
    if (t == 0)
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

std::vector<std::string> split(std::string &str, std::string &sep)
{
    return split(str.c_str(), sep.c_str());
}

std::vector<std::string> split(const char *str, const char *sep)
{
    int str_len = strlen(str);
    int sep_len = strlen(sep);
    std::vector<int> sep_index;

    for (int i = 0; i <= str_len - sep_len; )
    {
        bool is_sep = true;
        for (int j = i; j < i + sep_len; ++j)
        {
            if (str[j] != sep[j-i])
            {
                is_sep = false;
                break;
            }
        }
        if (is_sep)
        {
            sep_index.push_back(i);
            i += sep_len;
        }
        else
        {
            i++;
        }
    }
    if (sep_index.empty())
    {
        return { str };
    }
    std::vector<std::string> subs;
    for (int i = 0; i < static_cast<int>(sep_index.size()); ++i)
    {
        int begin, end = sep_index[i];
        if (i == 0)
        {
            begin = 0;
        }
        else
        {
            begin = sep_index[i-1] + sep_len;
        }
        subs.push_back(std::string(str, begin, end - begin));
        if (i == static_cast<int>(sep_index.size() - 1))
        {
            begin = sep_index[i] + sep_len;
            end = str_len;
            subs.push_back(std::string(str, begin, end - begin));
        }
    }
    return subs;
}

}   // namespace utils

}   // namespace cppev