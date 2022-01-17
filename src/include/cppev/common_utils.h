#ifndef _common_utils_h_6C0224787A17_
#define _common_utils_h_6C0224787A17_

#include <functional>

namespace cppev {

struct enum_hash {
    template <typename T>
    std::size_t operator()(const T &t) const {
        return std::hash<int>()((int)t);
    }
};

class uncopyable {
public:
    uncopyable() {}
    virtual ~uncopyable() noexcept {}
private:
    uncopyable &operator=(const uncopyable &) = delete;
    uncopyable(const uncopyable&) = delete;
};

void throw_logic_error(const char *str);

void throw_runtime_error(const char *str);

time_t sc_time();

std::string timestamp(time_t t = 0, const char *format = nullptr);

void ignore_signal(int sig);

}

#endif