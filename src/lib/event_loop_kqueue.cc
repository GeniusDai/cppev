#ifdef __APPLE__

#include <sys/event.h>

#include <cassert>
#include <ctime>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>
#include <tuple>

#include "cppev/common.h"
#include "cppev/event_loop.h"
#include "cppev/utils.h"

namespace cppev
{

using ev_type_of_kqueue = decltype(kevent::filter);
using ev_mode_of_kqueue = decltype(kevent::flags);

static ev_type_of_kqueue fd_event_map_wrapper_to_sys(fd_event ev)
{
    // EVFILT_READ and EVFILT_WRITE are exclusive!!!
    ev_type_of_kqueue flags = 0;
    if (static_cast<bool>(ev & fd_event::fd_readable))
    {
        flags = EVFILT_READ;
    }
    else if (static_cast<bool>(ev & fd_event::fd_writable))
    {
        flags = EVFILT_WRITE;
    }
    return flags;
}

static fd_event fd_event_map_sys_to_wrapper(ev_type_of_kqueue ev)
{
    // EVFILT_READ and EVFILT_WRITE are exclusive!!!
    fd_event flags = static_cast<fd_event>(0);
    if (ev == EVFILT_READ)
    {
        flags = fd_event::fd_readable;
    }
    else if (ev == EVFILT_WRITE)
    {
        flags = fd_event::fd_writable;
    }
    return flags;
}

static ev_mode_of_kqueue fd_mode_map_wrapper_to_sys(fd_event_mode mode)
{
    switch (static_cast<int>(mode))
    {
    case static_cast<int>(fd_event_mode::level_trigger):
        return 0;
    case static_cast<int>(fd_event_mode::edge_trigger):
        return EV_CLEAR;
    case static_cast<int>(fd_event_mode::oneshot):
        return EV_ONESHOT;
    default:
        throw_logic_error("Unknown mode");
    }
}

void event_loop::fd_io_multiplexing_create_nts()
{
    ev_fd_ = kqueue();
    if (ev_fd_ < 0)
    {
        throw_system_error("kqueue error");
    }
}

void event_loop::fd_io_multiplexing_add_nts(const std::shared_ptr<io> &iop,
                                            fd_event ev_type)
{
    LOG_DEBUG_FMT("Activate fd %d %s event", iop->fd(),
                  fd_event_to_string.at(ev_type));
    if (fd_event_masks_.count(iop->fd()) &&
        static_cast<bool>(fd_event_masks_[iop->fd()] & ev_type))
    {
        throw_logic_error("add existent event for fd ", iop->fd());
    }
    fd_event_masks_[iop->fd()] |= ev_type;
    struct kevent ev;
    ev_mode_of_kqueue ev_add_mode =
        EV_ADD | fd_mode_map_wrapper_to_sys(fd_event_modes_[iop->fd()]);
    //     &kev, ident, filter, flags, fflags, data, udata
    EV_SET(&ev, iop->fd(), fd_event_map_wrapper_to_sys(ev_type), ev_add_mode, 0,
           0, nullptr);
    if (kevent(ev_fd_, &ev, 1, nullptr, 0, nullptr) < 0)
    {
        throw_system_error("kevent add error for fd ", iop->fd());
    }
}

void event_loop::fd_io_multiplexing_del_nts(const std::shared_ptr<io> &iop,
                                            fd_event ev_type)
{
    LOG_DEBUG_FMT("Deactivate fd %d %s event", iop->fd(),
                  fd_event_to_string.at(ev_type));
    if (!(fd_event_masks_.count(iop->fd()) &&
          static_cast<bool>(fd_event_masks_[iop->fd()] & ev_type)))
    {
        throw_logic_error("delete nonexistent event for fd ", iop->fd());
    }
    fd_event_masks_[iop->fd()] ^= ev_type;
    if (!static_cast<bool>(fd_event_masks_[iop->fd()]))
    {
        fd_event_masks_.erase(iop->fd());
    }
    struct kevent ev;
    //     &kev, ident, filter, flags, fflags, data, udata
    EV_SET(&ev, iop->fd(), fd_event_map_wrapper_to_sys(ev_type), EV_DELETE, 0,
           0, nullptr);
    if (kevent(ev_fd_, &ev, 1, nullptr, 0, nullptr) < 0)
    {
        throw_system_error("kevent del error for fd ", iop->fd());
    }
}

std::vector<std::tuple<int, fd_event>> event_loop::fd_io_multiplexing_wait_ts(
    int timeout)
{
    int nums;
    struct kevent evs[sysconfig::event_number];
    if (timeout < 0)
    {
        nums =
            kevent(ev_fd_, nullptr, 0, evs, sysconfig::event_number, nullptr);
    }
    else
    {
        struct timespec ts;
        ts.tv_sec = timeout / 1000;
        ts.tv_nsec = (timeout % 1000) * 1000 * 1000;
        nums = kevent(ev_fd_, nullptr, 0, evs, sysconfig::event_number, &ts);
    }
    std::vector<std::tuple<int, fd_event>> fd_events;
    for (int i = 0; i < nums; ++i)
    {
        int fd = evs[i].ident;
        bool succeed = false;
        fd_event ev = fd_event_map_sys_to_wrapper(evs[i].filter);
        for (auto event : {fd_event::fd_readable, fd_event::fd_writable})
        {
            if (static_cast<bool>(ev & event))
            {
                succeed = true;
                fd_events.emplace_back(fd, event);
            }
        }
        if (!succeed)
        {
            LOG_ERROR_FMT("Kqueue event fd %d %d is invalid", fd, ev);
        }
    }
    return fd_events;
}

}  // namespace cppev

#endif  // event loop for macOS
