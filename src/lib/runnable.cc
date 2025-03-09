#include "cppev/runnable.h"

namespace cppev
{

runnable::runnable() : fut_(prom_.get_future())
{
}

runnable::~runnable() = default;

bool runnable::cancel() noexcept
{
    return 0 == pthread_cancel(thr_);
}

void runnable::run()
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

void runnable::join()
{
    int ret = pthread_join(thr_, nullptr);
    if (ret != 0)
    {
        throw_system_error("pthread_join error", ret);
    }
}

void runnable::detach()
{
    int ret = pthread_detach(thr_);
    if (ret != 0)
    {
        throw_system_error("pthread_detach error", ret);
    }
}

void runnable::send_signal(int sig) noexcept
{
    pthread_kill(thr_, sig);
}

}  // namespace cppev
