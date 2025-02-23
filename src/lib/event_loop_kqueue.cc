#include "cppev/event_loop.h"

#ifdef __APPLE__

#include <exception>
#include <memory>
#include <cassert>
#include <tuple>
#include <iostream>
#include <thread>
#include <ctime>
#include "cppev/utils.h"
#include "cppev/sysconfig.h"
#include <sys/event.h>

namespace cppev
{

static uint32_t fd_event_map_wrapper_to_sys(fd_event ev)
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

static fd_event fd_event_map_sys_to_wrapper(uint32_t ev)
{
    fd_event flags = static_cast<fd_event>(0);
    if (ev == EVFILT_READ)
    {
        flags = fd_event::fd_readable;
    }
    if (ev == EVFILT_WRITE)
    {
        flags = fd_event::fd_writable;
    }
    return flags;
}

event_loop::event_loop(void *data, void *owner)
: data_(data), owner_(owner), stop_(false)
{
    ev_fd_ = kqueue();
    if (ev_fd_ < 0)
    {
        throw_system_error("kqueue error");
    }
}

void event_loop::fd_io_multiplexing_add_nts(const std::shared_ptr<nio> &iop, fd_event ev_type)
{
    LOG_DEBUG_FMT("Activate %s for fd %d", fd_event_debug[ev_type], iop->fd());
    assert(fd_event_masks_.count(iop->fd()) ? !static_cast<bool>(fd_event_masks_[iop->fd()]&ev_type) : true);
    fd_event_masks_[iop->fd()] |= ev_type;
    // Register event to kqueue
    struct kevent ev;
    //     &kev, ident,     filter,                               flags,             fflags, data, udata);
    EV_SET(&ev,  iop->fd(), fd_event_map_wrapper_to_sys(ev_type), EV_ADD, 0,      0,    nullptr);
    if (kevent(ev_fd_, &ev, 1, nullptr, 0, nullptr) < 0)
    {
        throw_system_error(std::string("kevent add error for fd ").append(std::to_string(iop->fd())));
    }
}

void event_loop::fd_io_multiplexing_del_nts(const std::shared_ptr<nio> &iop, fd_event ev_type)
{
    LOG_DEBUG_FMT("Deactivate %s for fd %d", fd_event_debug[ev_type], iop->fd());
    if (!(fd_event_masks_.count(iop->fd()) && static_cast<bool>(fd_event_masks_[iop->fd()]&ev_type)))
    {
        throw_logic_error(std::string("delete nonexistent event for fd ").append(std::to_string(iop->fd())));
    }
    fd_event_masks_[iop->fd()] ^= ev_type;
    if (!static_cast<bool>(fd_event_masks_[iop->fd()]))
    {
        fd_event_masks_.erase(iop->fd());
    }
    // Remove event from kqueue
    struct kevent ev;
    //     &kev, ident,     filter,                               flags,     fflags, data, udata
    EV_SET(&ev,  iop->fd(), fd_event_map_wrapper_to_sys(ev_type), EV_DELETE, 0,      0,    nullptr);
    if (kevent(ev_fd_, &ev, 1, nullptr, 0, nullptr) < 0)
    {
        throw_system_error(std::string("kevent del error for fd ").append(std::to_string(iop->fd())));
    }
}

std::vector<std::tuple<int, fd_event>> event_loop::fd_io_multiplexing_wait_ts(int timeout)
{
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
    std::vector<std::tuple<int, fd_event>> fd_events;
    std::vector<fd_event> all_events{ fd_event::fd_readable, fd_event::fd_writable };
    for (int i = 0; i < nums; ++i)
    {
        int fd = evs[i].ident;
        for (int i = 0; i < all_events.size(); ++i)
        {
            if (static_cast<bool>(all_events[i] & fd_event_map_sys_to_wrapper(evs[i].filter)))
            {
                fd_events.emplace_back(fd, all_events[i]);
            }
        }
    }
    return fd_events;
}

}   // namespace cppev

#endif  // event loop for macOS
