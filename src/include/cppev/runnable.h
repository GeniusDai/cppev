#ifndef _runnable_h_6C0224787A17_
#define _runnable_h_6C0224787A17_

#include <exception>
#include <future>
#include <chrono>
#include <cstring>
#include <csignal>
#include <pthread.h>
#include "cppev/utils.h"

// Q1 : Why a new thread library ?
// A1 : std::thread doesn't support pthread_cancel and pthread_kill.

// Q2 : Is runnable a full encapsulation of pthread ?
// A2 : Remain components of pthread:
//      1) Per-Thread Context Routines : better use "thread_local".
//      2) Cleanup Routines : just coding in "run_impl".
//      3) Thread Routines : These routines are not essential
//         pthread_exit / pthread_once / pthread_self / pthread_equal

namespace cppev
{

class runnable
{
public:
    runnable()
    : fut_(prom_.get_future())
    {
    }

    runnable(const runnable &) = delete;
    runnable &operator=(const runnable &) = delete;
    runnable(runnable &&) = delete;
    runnable &operator=(runnable &&) = delete;

    virtual ~runnable() = default;

    // Derived class should override
    virtual void run_impl() = 0;

    // Create and run thread
    void run()
    {
        auto thr_func = [](void *arg) -> void *
        {
            runnable *pseudo_this = static_cast<runnable *>(arg);
            if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr) != 0)
            {
                throw_logic_error("pthread_setcancelstate error");
            }
            if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr) != 0)
            {
                throw_logic_error("pthread_setcanceltype error");
            }
            pseudo_this->run_impl();
            pseudo_this->prom_.set_value(true);
            return nullptr;
        };
        int ret = pthread_create(&thr_, nullptr, thr_func, this);
        if (ret != 0)
        {
            throw_system_error("pthread_create error", ret);
        }
    }

    // Wait until thread finish
    void join()
    {
        int ret = pthread_join(thr_, nullptr);
        if (ret != 0)
        {
            throw_system_error("pthread_join error", ret);
        }
    }

    // Detach thread
    void detach()
    {
        int ret = pthread_detach(thr_);
        if (ret != 0)
        {
            throw_system_error("pthread_detach error", ret);
        }
    }

    // Cancel thread
    virtual bool cancel() noexcept
    {
        return 0 == pthread_cancel(thr_);
    }

    // Wait for thread
    // @return  whether thread finishes
    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period> &span)
    {
        std::future_status stat = fut_.wait_for(span);
        bool ret = false;
        switch (stat)
        {
        case std::future_status::ready :
            ret = true;
            break;
        case std::future_status::timeout :
            ret = false;
            break;
        default:
            throw_logic_error("future::wait_for error");
            break;
        }
        return ret;
    }

    // Send signal to thread
    void send_signal(int sig) noexcept
    {
        pthread_kill(thr_, sig);
    }

private:
    pthread_t thr_;

    std::promise<bool> prom_;

    std::future<bool> fut_;
};

}   // namespace cppev

#endif  // runnable.h