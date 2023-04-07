#ifndef _event_loop_h_6C0224787A17_
#define _event_loop_h_6C0224787A17_

#include <unordered_map>
#include <memory>
#include <unistd.h>
#include <queue>
#include <tuple>
#include <mutex>
#include <functional>
#include "cppev/nio.h"
#include "cppev/sysconfig.h"
#include "cppev/utils.h"
#ifdef CPPEV_DEBUG
#include "cppev/async_logger.h"
#endif  // CPPEV_DEBUG

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

using fd_event_handler = std::function<void(const std::shared_ptr<nio> &)>;

class event_loop
{
public:
    explicit event_loop(void *data = nullptr, void *back = nullptr);

    event_loop(const event_loop &) = delete;
    event_loop &operator=(const event_loop &) = delete;
    event_loop(event_loop &&) = delete;
    event_loop &operator=(event_loop &&) = delete;

    virtual ~event_loop() noexcept
    {
        close(ev_fd_);
    }

    int ev_fd() const noexcept
    {
        return ev_fd_;
    }

    void *data() const noexcept
    {
        return data_;
    }

    void *back() const noexcept
    {
        return back_;
    }

    int ev_loads() const noexcept
    {
        return fds_.size();
    }

    // Register fd event to event pollor
    // @param iop       nio smart pointer
    // @param ev_type   event type
    // @param handler   fd event handler
    // @param activate  whether register fd to os io-multiplexing api
    // @param prio      event priority
    void fd_register(const std::shared_ptr<nio> &iop, fd_event ev_type,
        const fd_event_handler &handler = fd_event_handler(), bool activate = true, priority prio = p0);

    // Remove fd event(s) from event pollor
    // @param iop           nio smart pointer
    // @param clean         whether clean callbacks stored in eventloop
    // @param deactivate    whether remove fd from os io-multiplexing api (parameter is provided due to
    //                      the io-multiplexing api may cause program get killed when fd is closed)
    void fd_remove(const std::shared_ptr<nio> &iop, bool clean = true, bool deactivate = true);

    // Wait for events, only loop once, timeout unit is millisecond
    void loop_once(int timeout = -1);

    // Loop infinitely
    void loop_forever(int timeout = -1)
    {
        while(true)
        {
            loop_once(timeout);
        }
    }

private:
    // Used for registering callback for event loop
    std::mutex lock_;

    // Event watcher fd
    int ev_fd_;

    // External data for eventloop
    void *data_;

    // External class contains eventloop
    void *back_;

    // Tuple : priority, nio, callback, event
    std::unordered_multimap<int, std::tuple<int, std::shared_ptr<nio>, std::shared_ptr<fd_event_handler>, fd_event>> fds_;

    // Tuple : priority, nio, callback
    std::priority_queue<std::tuple<int, std::shared_ptr<nio>, std::shared_ptr<fd_event_handler>> > fd_cbs_;

    // Activate events, used for kqueue only
    std::unordered_map<int, fd_event> fd_events_;
};

}   // namespace cppev

#endif  // event_loop.h
