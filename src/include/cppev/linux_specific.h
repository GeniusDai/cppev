#ifndef _linux_specific_h_6C0224787A17_
#define _linux_specific_h_6C0224787A17_

#ifdef __linux__

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <ctime>
#include <csignal>
#include <sys/inotify.h>
#include "cppev/nio.h"

namespace cppev
{

class spinlock final
{
public:
    spinlock()
    {
        if (pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE) != 0)
        {
            throw_system_error("pthread_spin_init error");
        }
    }

    spinlock(const spinlock &&) = delete;
    spinlock &operator=(const spinlock &&) = delete;
    spinlock(spinlock &&) = delete;
    spinlock &operator=(spinlock &&) = delete;

    ~spinlock()
    {
        if (pthread_spin_destroy(&lock_) != 0)
        {
            throw_system_error("pthread_spin_destroy error");
        }
    }

    void lock()
    {
        if (pthread_spin_lock(&lock_) != 0)
        {
            throw_system_error("pthread_spin_lock error");
        }
    }

    void unlock()
    {
        if (pthread_spin_unlock(&lock_) != 0)
        {
            throw_system_error("pthread_spin_unlock error");
        }
    }

    bool trylock()
    {
        int ret = pthread_spin_trylock(&lock_);
        if (ret == 0)
        {
            return true;
        }
        else if (ret == EBUSY)
        {
            return false;
        }
        throw_system_error("pthread_spin_trylock error");
        return ret;
    }

private:
    pthread_spinlock_t lock_;
};

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

    ~timer() = default;

    void stop();

private:
    timer_handler handler_;

    timer_t tmid_;
};

template<typename Rep, typename Period>
timer::timer(const std::chrono::duration<Rep, Period> &span, const timer_handler &handler)
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
    auto interval = std::chrono::nanoseconds(span).count();
    its.it_interval.tv_sec = interval / (1000 * 1000 * 1000);
    its.it_interval.tv_nsec = interval % (1000 * 1000 * 1000);
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

#endif  // linux_specific.h
