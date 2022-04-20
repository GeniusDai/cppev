#ifndef _event_loop_h_6C0224787A17_
#define _event_loop_h_6C0224787A17_

#include <unordered_map>
#include <memory>
#include <unistd.h>
#include <queue>
#include <tuple>
#include <mutex>
#include "cppev/nio.h"
#include "cppev/sysconfig.h"
#include "cppev/common_utils.h"

namespace cppev
{

enum class fd_event
{
    fd_readable = 1 << 0,
    fd_writable = 1 << 1,
};

constexpr fd_event operator&(fd_event a, fd_event b)
{
    return static_cast<fd_event>(static_cast<int>(a) & static_cast<int>(b));
}

constexpr fd_event operator|(fd_event a, fd_event b)
{
    return static_cast<fd_event>(static_cast<int>(a) | static_cast<int>(b));
}

enum priority
{
    low = 10,
    high = 20
};

class event_loop;

typedef void (*fd_event_cb)(std::shared_ptr<nio> iop);

typedef void (*ev_handler)(event_loop *);

class event_loop
: public uncopyable
{
public:
    explicit event_loop(void *data = nullptr);

    virtual ~event_loop()
    {
        close(ev_fd_);
    }

    int ev_fd() const
    {
        return ev_fd_;
    }

    void *data()
    {
        return data_;
    }

    // Loads of event loop
    int fd_loads() const
    {
        return fds_.size();
    }

    // Register fd to event pollor, default activate callback
    void fd_register(std::shared_ptr<nio> iop, fd_event ev_type,
        fd_event_cb ev_cb = nullptr, bool activate = true, priority prio = low);

    // Remove fd from event pollor, default clean callback
    void fd_remove(std::shared_ptr<nio> iop, bool clean = true);

    // Wait for events, only loop once, timeout unit is millisecond
    void loop_once(int timeout = -1);

    // Loop infinitely
    void loop(int timeout = -1)
    {
        while(true)
        {
            on_loop_(this);
            loop_once();
        }
    }

private:
    // When event loop starts;
    ev_handler on_loop_;

    // Used for registering callback for event loop
    std::mutex lock_;

    // Event watcher fd
    int ev_fd_;

    // Thread pool shared data
    void *data_;

    // Tuple : priority, nio, callback, event
    std::unordered_multimap<int, std::tuple<int, std::shared_ptr<nio>, fd_event_cb, fd_event> > fds_;

    // Tuple : priority, nio, callback
    std::priority_queue<std::tuple<int, std::shared_ptr<nio>, fd_event_cb> > fd_cbs_;

    // Activate events, used for kqueue only
    std::unordered_map<int, fd_event> fd_events_;
};

}   // namespace cppev

#endif  // event_loop.h
