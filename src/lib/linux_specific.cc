#include "cppev/linux_specific.h"

#ifdef __linux__

namespace cppev
{

timer::timer(int interval, const timer_handler &handler)
: handler_(handler)
{
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
    its.it_interval.tv_sec = interval / 1000;
    its.it_interval.tv_nsec = (interval % 1000) * 1000 * 1000;
    if (timer_settime(tmid_, 0, &its, nullptr) != 0)
    {
        throw_system_error("timer_settime error");
    }
}

void timer::stop()
{
    if (timer_delete(tmid_) != 0)
    {
        throw_system_error("timer_delete error");
    }
}

}   // namespace cppev

#endif  // __linux__
