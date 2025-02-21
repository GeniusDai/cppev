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

static uint32_t fd_event_map_wrapper_to_sys(fd_event ev)
{
    int flags = 0;
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

static fd_event fd_event_map_sys_to_wrapper(uint32_t ev)
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

void event_loop::fd_register(const std::shared_ptr<nio> &iop, fd_event ev_type,
    const fd_event_handler &handler, bool activate, priority prio)
{
#ifdef CPPEV_DEBUG
    PRINT_FD_REGISTER_DEBUG();
#endif  // CPPEV_DEBUG
    iop->set_evlp(*this);
    if (handler)
    {
        std::unique_lock<std::mutex> lock(lock_);
        fds_.emplace(iop->fd(), std::make_tuple(prio, iop, std::make_shared<fd_event_handler>(handler), ev_type));
    }
    if (activate)
    {
        struct epoll_event ev;
        ev.data.fd = iop->fd();
        ev.events = fd_event_map_wrapper_to_sys(ev_type);
        if (epoll_ctl(ev_fd_, EPOLL_CTL_ADD, iop->fd(), &ev) < 0)
        {
            throw_system_error(std::string("epoll_ctl add error for fd ").append(std::to_string(iop->fd())));
        }
    }
}

void event_loop::fd_remove(const std::shared_ptr<nio> &iop, bool clean, bool deactivate)
{
#ifdef CPPEV_DEBUG
    PRINT_FD_REMOVE_DEBUG();
#endif  // CPPEV_DEBUG
    if (deactivate)
    {
        if (epoll_ctl(ev_fd_, EPOLL_CTL_DEL, iop->fd(), nullptr) < 0)
        {
            throw_system_error(std::string("epoll_ctl del error for fd ").append(std::to_string(iop->fd())));
        }
    }
    if (clean)
    {
        std::unique_lock<std::mutex> lock(lock_);
        fds_.erase(iop->fd());
    }
}

void event_loop::loop_once(int timeout)
{
    // 1. Add to priority queue
    epoll_event evs[sysconfig::event_number];
    int nums = epoll_wait(ev_fd_, evs, sysconfig::event_number, timeout);
    if (nums < 0 && errno != EINTR)
    {
        throw_system_error("epoll_wait error");
    }
    std::priority_queue<std::tuple<priority, std::shared_ptr<nio>, std::shared_ptr<fd_event_handler>> > fd_callbacks;
    {
        std::unique_lock<std::mutex> lock(lock_);
        for (int i = 0; i < nums; ++i)
        {
            int fd = evs[i].data.fd;
            auto range = fds_.equal_range(fd);
            auto begin = range.first, end = range.second;
            while (begin != end)
            {
                if (static_cast<bool>(std::get<3>(begin->second) & fd_event_map_sys_to_wrapper(evs[i].events)))
                {
#ifdef CPPEV_DEBUG
                    PRINT_LOOP_DEBUG();
#endif  //  CPPEV_DEBUG
                    fd_callbacks.emplace(
                        std::get<0>(begin->second),
                        std::get<1>(begin->second),
                        std::get<2>(begin->second)
                    );
                }
                ++begin;
            }
        }
    }

    // 2. Pop from priority queue
    while (fd_callbacks.size())
    {
        auto ev = fd_callbacks.top();
        fd_callbacks.pop();
        (*std::get<2>(ev))(std::get<1>(ev));
    }

}

}   // namespace cppev

#endif  // event loop for linux
