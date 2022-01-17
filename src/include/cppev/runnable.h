#ifndef _runnable_h_6C0224787A17_
#define _runnable_h_6C0224787A17_

#include <exception>
#include <pthread.h>
#include <future>
#include <chrono>
#include "cppev/common_utils.h"

namespace cppev {

class runnable : public uncopyable {
public:
    runnable() { fut_ = prom_.get_future(); }

    virtual ~runnable() {};

    // Derived class should override
    virtual void run_impl() = 0;

    // Create and run thread
    void run() {
        auto thr_func = [](void *arg) -> void *{
            static_cast<runnable *>(arg)->run_impl();
            static_cast<runnable *>(arg)->prom_.set_value(true);
            return nullptr;
        };
        if (pthread_create(&thr_, nullptr, thr_func, this) != 0)
        { throw_runtime_error("pthread_create error"); }
    }

    // Wait until thread finish
    void join() {
        if (pthread_join(thr_, nullptr) != 0)
        { throw_runtime_error("pthread_join error"); }
    }

    // Cancel thread
    void cancel() {
        if (pthread_cancel(thr_) != 0)
        { throw_runtime_error("pthread_cancel error"); }
        join();
    }

    // Wait for thread
    // @Ret: whether thread finishes
    bool wait_for(std::chrono::milliseconds span) {
        std::future_status stat = fut_.wait_for(span);
        bool ret;
        switch (stat) {
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

}

#endif