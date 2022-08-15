#ifdef __linux__

#ifndef _linux_specific_h_6C0224787A17_
#define _linux_specific_h_6C0224787A17_

#include <memory>
#include <string>
#include <unordered_map>
#include <ctime>
#include <csignal>
#include <sys/inotify.h>
#include "cppev/nio.h"

namespace cppev
{

using fs_event_handler = std::function<void(inotify_event *, const std::string &)>;

class nwatcher final
: public nstream
{
public:
    explicit nwatcher(int fd, fs_event_handler handler = fs_event_handler())
    : nio(fd), nstream(fd), handler_(handler)
    {}

    void set_handler(const fs_event_handler &handler)
    {
        handler_ = handler;
    }

    void add_watch(const std::string &path, uint32_t events);

    void del_watch(const std::string &path);

    void process_events();

private:
    std::unordered_map<int, std::string> wds_;

    std::unordered_map<std::string, int> paths_;

    fs_event_handler handler_;
};

namespace nio_factory
{

std::shared_ptr<nwatcher> get_nwatcher();

} // namespace nio_factory

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
    using timer_handler = std::function<void(void *)>;

    explicit timer(int interval, const timer_handler &handler, void *data = nullptr);

    timer(const timer &) = delete;
    timer &operator=(const timer &) = delete;
    timer(timer &&) = delete;
    timer &operator=(timer &&) = delete;

    ~timer() = default;

    void stop();

private:
    timer_handler handler_;

    void *data_;

    timer_t tmid_;
};

}   // namespace cppev

#endif  // linux_specific.h

#endif  // __linux__
