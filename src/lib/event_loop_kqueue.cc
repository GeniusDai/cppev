#ifdef __APPLE__

#include <exception>
#include <memory>
#include <cassert>
#include <tuple>
#include <iostream>
#include <thread>
#include <ctime>
#include "cppev/event_loop.h"
#include "cppev/common_utils.h"
#include "cppev/sysconfig.h"
#include "cppev/async_logger.h"
#include <sys/event.h>

namespace cppev
{

ev_handler idle_ev_handler = [](event_loop *) -> void {};

static uint32_t fd_map_to_sys(fd_event ev)
{
    int flags = 0;
    if (static_cast<bool>(ev & fd_event::fd_readable))
    {
        flags |= EVFILT_READ;
    }
    if (static_cast<bool>(ev & fd_event::fd_writable))
    {
        flags |= EVFILT_WRITE;
    }
    return flags;
}

static fd_event fd_map_to_event(uint32_t ev)
{
    fd_event flags = static_cast<fd_event>(0);
    if (ev & EVFILT_READ)
    {
        flags = flags | fd_event::fd_readable;
    }
    if (ev & EVFILT_WRITE)
    {
        flags = flags | fd_event::fd_writable;
    }
    return flags;
}

event_loop::event_loop(void *data)
: data_(data)
{
    ev_fd_ = kqueue();
    if (ev_fd_ < 0)
    {
        throw_system_error("kqueue error");
    }
    on_loop_ = idle_ev_handler;
}

void event_loop::fd_register(std::shared_ptr<nio> iop, fd_event ev_type,
    fd_event_cb ev_cb, bool activate, priority prio)
{
    log::info << "[Action:register] ";
    log::info << "[Fd:" << iop->fd() << "] ";

    log::info << "[Event:";
    if (static_cast<bool>(ev_type & fd_event::fd_readable))
    {
        log::info << "readable] ";
    }
    if (static_cast<bool>(ev_type & fd_event::fd_writable))
    {
        log::info << "writable] ";
    }

    log::info << "[Callback:";
    if (ev_cb)
    {
        log::info << "not-null] ";
    }
    else
    {
        log::info << "null] ";
    }

    log::info << "[Activate:";
    if (activate)
    {
        log::info << "true]";
    }
    else
    {
        log::info << "false]";
    }

    log::info << log::endl;

    iop->set_evlp(this);
    if (ev_cb)
    {
        std::unique_lock<std::mutex> lock(lock_);
        fds_.emplace(iop->fd(), std::tuple<int, std::shared_ptr<nio>,
            fd_event_cb, fd_event>(prio, iop, ev_cb, ev_type));
    }
    if (activate)
    {
        // Record event that has been register to kqueue
        {
            std::unique_lock<std::mutex> lock(lock_);
            if (fd_events_.count(iop->fd()))
            {
                fd_events_[iop->fd()] = fd_events_[iop->fd()] | ev_type;
            }
            else
            {
                fd_events_[iop->fd()] = ev_type;
            }
        }

        // Register event to kqueue
        struct kevent ev;
        // EV_SET(&kev, ident, filter, flags, fflags, data, udata);
        EV_SET(&ev, iop->fd(), fd_map_to_sys(ev_type) , EV_ADD | EV_CLEAR, 0, 0, nullptr);
        if (kevent(ev_fd_, &ev, 1, nullptr, 0, nullptr) < 0)
        {
            throw_system_error("kevent error");
        }
    }
}

void event_loop::fd_remove(std::shared_ptr<nio> iop, bool clean)
{
    log::info << "[Action:remove] ";
    log::info << "[Fd:" << iop->fd() << "] ";
    log::info << "[Clean:";
    if (clean)
    {
        log::info << "true]";
    }
    else
    {
        log::info << "false]";
    }
    log::info << log::endl;

    // Remove event records
    fd_event ev_type;
    {
        std::unique_lock<std::mutex> lock(lock_);
        ev_type = fd_events_[iop->fd()];
        fd_events_.erase(iop->fd());
    }

    // Remove event from kqueue
    struct kevent ev;
    // EV_SET(&kev, ident, filter, flags, fflags, data, udata);
    EV_SET(&ev, iop->fd(), fd_map_to_sys(ev_type), EV_DELETE, 0, 0, nullptr);
    if (kevent(ev_fd_, &ev, 1, nullptr, 0, nullptr) < 0)
    {
        throw_system_error("kevent error");
    }

    if (clean)
    {
        std::unique_lock<std::mutex> lock(lock_);
        fds_.erase(iop->fd());
    }
}

void event_loop::loop_once(int timeout)
{
    log::info << "start event loop" << log::endl;
    // 1. Add to priority queue
    int nums;
    struct kevent evs[sysconfig::event_number];
    if (timeout < 0)
    {
        nums = kevent(ev_fd_, nullptr, 0, evs, sysconfig::event_number, nullptr);
    }
    else
    {
        struct timespec ts;
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000 * 1000;
        nums = kevent(ev_fd_, nullptr, 0, evs, sysconfig::event_number, &ts);
    }
    for (int i = 0; i < nums; ++i)
    {
        std::unique_lock<std::mutex> lock(lock_);
        int fd = evs[i].ident;
        auto range = fds_.equal_range(fd);
        auto begin = range.first, end = range.second;
        while (begin != end)
        {
            if (static_cast<bool>(std::get<3>(begin->second)
                & fd_map_to_event(evs[i].flags)))
            {
#ifdef CPPEV_DEBUG
                log::info << "enqueue ";
                if (static_cast<bool>(std::get<3>(begin->second) & fd_event::fd_readable))
                {
                    log::info << "readable event ";
                }
                if (static_cast<bool>(std::get<3>(begin->second) & fd_event::fd_writable))
                {
                    log::info << "writable event ";
                }
                log::info << "for fd " << fd << log::endl;
#endif  //  CPPEV_DEBUG
                fd_cbs_.emplace(
                    std::get<0>(begin->second),
                    std::get<1>(begin->second),
                    std::get<2>(begin->second)
                );
            }
            ++begin;
        }
    }
    // 2. Pop from priority queue
    while (fd_cbs_.size())
    {
        auto ev = fd_cbs_.top();
        fd_cbs_.pop();
        (std::get<2>(ev))(std::get<1>(ev));
    }

}

}   // namespace cppev

#endif  // event loop for macOS
