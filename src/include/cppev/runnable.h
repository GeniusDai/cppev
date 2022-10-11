#ifndef _runnable_h_6C0224787A17_
#define _runnable_h_6C0224787A17_

#include <exception>
#include <pthread.h>
#include <future>
#include <chrono>
#include <cstring>
#include "cppev/common_utils.h"

// Q1 : Why a new thread library ?
// A1 : std::thread doesn't provide support for cancel.

// Q2 : Is runnable a full encapsulation of pthread ?
// A2 : Remain components of pthread:
//      1) pthread_key : better use "thread_local".
//      2) pthread_cleanup : just coding in "run_impl".
//      3) pthread_exit : use "return" and add returncode to your class if needed.
//      4) pthread_sigmask : OMG! Deep use of signal is never a good idea!

namespace cppev
{

class runnable
{
public:
    runnable()
    : fut_(prom_.get_future())
    {}

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
            pseudo_this->run_impl();
            pseudo_this->prom_.set_value(true);
            return nullptr;
        };
        if (pthread_create(&thr_, nullptr, thr_func, this) != 0)
        {
            throw_system_error("pthread_create error");
        }
    }

    // Wait until thread finish
    void join() const
    {
        if (pthread_join(thr_, nullptr) != 0)
        {
            throw_system_error("pthread_join error");
        }
    }

    // Detach thread
    void detach() const
    {
        if (pthread_detach(thr_) != 0)
        {
            throw_system_error("pthread_detach error");
        }
    }

    // Cancel thread
    void cancel() const
    {
        if (pthread_cancel(thr_) != 0)
        {
            throw_system_error("pthread_cancel error");
        }
    }

    // Wait for thread
    // @Ret: whether thread finishes
    bool wait_for(const std::chrono::milliseconds &span)
    {
        std::future_status stat = fut_.wait_for(span);
        bool ret = false;
        switch (stat)
        {
        case std::future_status::ready :
            join();
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

private:
    pthread_t thr_;

    std::promise<bool> prom_;

    std::future<bool> fut_;
};

}   // namespace cppev

#endif  // runnable.h