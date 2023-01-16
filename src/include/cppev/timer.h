#ifndef _timer_h_6C0224787A17_
#define _timer_h_6C0224787A17_

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <ctime>
#include <csignal>
#include "cppev/utils.h"

namespace cppev
{

class timer final
{
public:
    using timer_handler = std::function<void(void)>;

    template <typename Rep, typename Period>
    explicit timer(const std::chrono::duration<Rep, Period> &interval, const timer_handler &handler);

    timer(const timer &) = delete;
    timer &operator=(const timer &) = delete;
    timer(timer &&) = delete;
    timer &operator=(timer &&) = delete;

    ~timer() noexcept;

    void stop();

private:
    timer_handler handler_;

#ifdef __linux__
    timer_t tmid_;
#else
    bool stop_;

    std::chrono::time_point<std::chrono::system_clock> curr_;

    std::chrono::nanoseconds span_;

    std::thread thr_;
#endif
};

template<typename Rep, typename Period>
timer::timer(const std::chrono::duration<Rep, Period> &span, const timer_handler &handler)
: handler_(handler)
{
#ifdef __linux__
    sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_value.sival_ptr = this;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = [](sigval value) -> void
    {
        timer *pseudo_this = reinterpret_cast<timer *>(value.sival_ptr);
        pseudo_this->handler_();
    };
    if (timer_create(CLOCK_REALTIME, &sev, &tmid_) != 0)
    {
        throw_system_error("timer_create error");
    }
    itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;
    auto interval = std::chrono::nanoseconds(span).count();
    its.it_interval.tv_sec = interval / (1000 * 1000 * 1000);
    its.it_interval.tv_nsec = interval % (1000 * 1000 * 1000);
    if (timer_settime(tmid_, 0, &its, nullptr) != 0)
    {
        throw_system_error("timer_settime error");
    }
#else
    stop_ = false;
    curr_ = std::chrono::system_clock::now();
    span_ = span;
    auto thr_func = [this]()
    {
        while(!stop_)
        {
            curr_ += std::chrono::duration_cast<decltype(curr_)::duration>(span_);
            std::this_thread::sleep_until(curr_);
            handler_();
        }
    };
    thr_ = std::thread(thr_func);
#endif
}

timer::~timer() noexcept
{
#ifndef __linux__
    thr_.join();
#endif
}

void timer::stop()
{
#ifdef __linux__
    if (timer_delete(tmid_) != 0)
    {
        throw_system_error("timer_delete error");
    }
#else
    stop_ = true;
#endif
}

}   // namespace cppev

#endif  // timer.h
