#ifndef _cppev_runnable_h_6C0224787A17_
#define _cppev_runnable_h_6C0224787A17_

#include <pthread.h>

#include <chrono>
#include <csignal>
#include <cstring>
#include <exception>
#include <future>

#include "cppev/utils.h"

/*
    Q1 : Why a new thread library ?
    A1 : Previously std::thread doesn't support pthread_cancel and pthread_kill,
         but now std::jthread is recommended. Or if you prefer subthread
         implemented by subclass.

    Q2 : Is runnable a full encapsulation of pthread ?
    A2 : Remain components of pthread:
        1) Per-Thread Context Routines : better use "thread_local".
        2) Cleanup Routines : just coding in "run_impl".
        3) Thread Routines : These routines are not essential
            pthread_exit / pthread_once / pthread_self / pthread_equal
 */

namespace cppev
{

class CPPEV_PUBLIC runnable
{
public:
    runnable();

    runnable(const runnable &) = delete;
    runnable &operator=(const runnable &) = delete;
    runnable(runnable &&) = delete;
    runnable &operator=(runnable &&) = delete;

    virtual ~runnable();

    // Derived class should override
    virtual void run_impl() = 0;

    // Cancel thread
    virtual bool cancel() noexcept;

    // Create and run thread
    void run();

    // Wait until thread finish
    void join();

    // Detach thread
    void detach();

    // Send signal to thread
    void send_signal(int sig) noexcept;

    // Wait for thread
    // @return  whether thread finishes
    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period> &span)
    {
        std::future_status stat = fut_.wait_for(span);
        bool ret = false;
        switch (stat)
        {
        case std::future_status::ready:
            ret = true;
            break;
        case std::future_status::timeout:
            ret = false;
            break;
        default:
            throw_logic_error("future::wait_for error");
            break;
        }
        return ret;
    }

private:
    pthread_t thr_;

    std::promise<bool> prom_;

    std::future<bool> fut_;
};

}  // namespace cppev

#endif  // runnable.h
