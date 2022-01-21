#include "cppev/common_utils.h"
#include <string>
#include <cstring>
#include <exception>
#include <system_error>
#include <signal.h>

namespace cppev {

void throw_system_error(const char *str) {
    throw std::system_error(std::error_code(errno, std::system_category()) ,str);
}

void throw_logic_error(const char *str) {
    throw std::logic_error(str);
}

void throw_runtime_error(const char *str) {
    throw std::runtime_error( str);
}

time_t sc_time() {
    time_t t = time(NULL);
    if (t == -1) { throw_system_error("time error"); }
    return t;
}

std::string timestamp(time_t t, const char *format) {
    if (t == 0) { t = sc_time(); }
    if (format == nullptr) { format = "%F %T %Z"; }
    tm s_tm;
    localtime_r(&t, &s_tm);
    char buf[1024];
    memset(buf, 0, 1024);
    if (0 == strftime(buf, 1024, format, &s_tm))
    { throw_system_error("strftime error"); }
    return buf;
}

void ignore_signal(int sig) {
    struct sigaction sigact;
    memset(&sigact, 0, sizeof(sigact));
    sigact.sa_handler = SIG_IGN;
    if (sigaction(sig, &sigact, nullptr) == -1)
    { throw_system_error("sigaction error"); }
}

}