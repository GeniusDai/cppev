#include "cppev/event_loop.h"

#ifdef __linux__

#include <exception>
#include <memory>
#include <cassert>
#include <tuple>
#include <iostream>
#include <thread>
#include "cppev/utils.h"
#include "cppev/sysconfig.h"
#include <sys/epoll.h>

namespace cppev
{

using ev_type_of_epoll = decltype(epoll_event::events);

static ev_type_of_epoll fd_event_map_wrapper_to_sys(fd_event ev)
{
    ev_type_of_epoll flags = 0;
    if (static_cast<bool>(ev & fd_event::fd_readable))
    {
        flags |= EPOLLIN;
    }
    if (static_cast<bool>(ev & fd_event::fd_writable))
    {
        flags |= EPOLLOUT;
    }
    return flags;
}

static fd_event fd_event_map_sys_to_wrapper(ev_type_of_epoll ev)
{
    fd_event flags = static_cast<fd_event>(0);
    if (ev & EPOLLIN)
    {
        flags = flags | fd_event::fd_readable;
    }
    if (ev & EPOLLOUT)
    {
        flags = flags | fd_event::fd_writable;
    }
    return flags;
}

event_loop::event_loop(void *data, void *owner)
: data_(data), owner_(owner), stop_(false)
{
    ev_fd_ = epoll_create(sysconfig::event_number);
    if (ev_fd_ < 0)
    {
        throw_system_error("epoll_create error");
    }
}

void event_loop::fd_io_multiplexing_add_nts(const std::shared_ptr<nio> &iop, fd_event ev_type)
{
    LOG_DEBUG_FMT("Activate fd %d %s event", iop->fd(), fd_event_debug[ev_type]);
    auto ep_ctl = EPOLL_CTL_ADD;
    if (fd_event_masks_.count(iop->fd()))
    {
        if (static_cast<bool>(fd_event_masks_[iop->fd()]&ev_type))
        {
            throw_logic_error(std::string("add existent event for fd ").append(std::to_string(iop->fd())));
        }
        ep_ctl = EPOLL_CTL_MOD;
    }
    fd_event_masks_[iop->fd()] |= ev_type;
    struct epoll_event ev;
    ev.data.fd = iop->fd();
    ev.events = fd_event_map_wrapper_to_sys(fd_event_masks_[iop->fd()]);
    // LOG_DEBUG_FMT("Mod or add events to %d for fd %d", ev.events, iop->fd());
    if (epoll_ctl(ev_fd_, ep_ctl, iop->fd(), &ev) < 0)
    {
        std::unordered_map<int, std::string> err_hash = {
            { EPOLL_CTL_ADD, "EPOLL_CTL_ADD" },
            { EPOLL_CTL_MOD, "EPOLL_CTL_MOD" },
        };
        throw_system_error(std::string(err_hash[ep_ctl]).append(" error for fd ").append(std::to_string(iop->fd())));
    }
}

void event_loop::fd_io_multiplexing_del_nts(const std::shared_ptr<nio> &iop, fd_event ev_type)
{
    LOG_DEBUG_FMT("Deactivate fd %d %s event", iop->fd(), fd_event_debug[ev_type]);
    if (!(fd_event_masks_.count(iop->fd()) && static_cast<bool>(fd_event_masks_[iop->fd()]&ev_type)))
    {
        throw_logic_error(std::string("delete nonexistent event for fd ").append(std::to_string(iop->fd())));
    }
    fd_event_masks_[iop->fd()] ^= ev_type;
    if (!static_cast<bool>(fd_event_masks_[iop->fd()]))
    {
        fd_event_masks_.erase(iop->fd());
    }
    if (fd_event_masks_.count(iop->fd()))
    {
        struct epoll_event ev;
        ev.data.fd = iop->fd();
        ev.events = fd_event_map_wrapper_to_sys(fd_event_masks_[iop->fd()]);
        // LOG_DEBUG_FMT("Mod events to %d for fd %d", ev.events, iop->fd());
        if (epoll_ctl(ev_fd_, EPOLL_CTL_MOD, iop->fd(), &ev) < 0)
        {
            throw_system_error(std::string("EPOLL_CTL_MOD error for fd ").append(std::to_string(iop->fd())));
        }
    }
    else
    {
        // LOG_DEBUG_FMT("Delete all events for fd %d", iop->fd());
        if (epoll_ctl(ev_fd_, EPOLL_CTL_DEL, iop->fd(), nullptr) < 0)
        {
            throw_system_error(std::string("EPOLL_CTL_DEL error for fd ").append(std::to_string(iop->fd())));
        }
    }
}

std::vector<std::tuple<int, fd_event>> event_loop::fd_io_multiplexing_wait_ts(int timeout)
{
    epoll_event evs[sysconfig::event_number];
    int nums = epoll_wait(ev_fd_, evs, sysconfig::event_number, timeout);
    if (nums < 0 && errno != EINTR)
    {
        throw_system_error("epoll_wait error");
    }
    std::vector<std::tuple<int, fd_event>> fd_events;
    for (int i = 0; i < nums; ++i)
    {
        int fd = evs[i].data.fd;
        bool succeed = false;
        fd_event ev = fd_event_map_sys_to_wrapper(evs[i].events);
        for (auto event : { fd_event::fd_readable, fd_event::fd_writable })
        {
            if (static_cast<bool>(ev & event))
            {
                succeed = true;
                fd_events.emplace_back(fd, event);
            }
        }
        if (!succeed)
        {
            LOG_ERROR_FMT("Epoll event fd %d %d is invalid", fd, ev);
        }
    }
    return fd_events;
}

}   // namespace cppev

#endif  // event loop for linux
