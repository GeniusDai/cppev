#include "cppev/event_loop.h"

namespace cppev
{

std::unordered_map<fd_event, const char *> fd_event_debug = {
    { fd_event::fd_readable, "fd_readable" },
    { fd_event::fd_writable, "fd_writable" },
};

event_loop::~event_loop() noexcept
{
    close(ev_fd_);
}

void *event_loop::data() noexcept
{
    return data_;
}

const void *event_loop::data() const noexcept
{
    return data_;
}

void *event_loop::owner() noexcept
{
    return owner_;
}

const void *event_loop::owner() const noexcept
{
    return owner_;
}

int event_loop::ev_loads() const noexcept
{
    return fd_event_datas_.size();
}

void event_loop::fd_register(const std::shared_ptr<nio> &iop, fd_event ev_type,
    const fd_event_handler &handler, priority prio)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_register_nts(iop, ev_type, handler, prio);
}

void event_loop::fd_activate(const std::shared_ptr<nio> &iop, fd_event ev_type)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_io_multiplexing_add_nts(iop, ev_type);
}

void event_loop::fd_register_and_activate(const std::shared_ptr<nio> &iop, fd_event ev_type,
    const fd_event_handler &handler, priority prio)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_register_nts(iop, ev_type, handler, prio);
    fd_io_multiplexing_add_nts(iop, ev_type);
}

void event_loop::fd_remove(const std::shared_ptr<nio> &iop, fd_event ev_type)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_remove_nts(iop, ev_type);
}

void event_loop::fd_deactivate(const std::shared_ptr<nio> &iop, fd_event ev_type)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_io_multiplexing_del_nts(iop, ev_type);
}

void event_loop::fd_remove_and_deactivate(const std::shared_ptr<nio> &iop, fd_event ev_type)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_io_multiplexing_del_nts(iop, ev_type);
    fd_remove_nts(iop, ev_type);
}

void event_loop::fd_remove_and_deactivate_all(const std::shared_ptr<nio> &iop)
{
    std::unique_lock<std::mutex> lock(lock_);
    for (auto ev : { fd_event::fd_readable, fd_event::fd_writable })
    {
        if (fd_event_masks_.count(iop->fd()) && static_cast<bool>(ev & fd_event_masks_[iop->fd()]))
        {
            fd_io_multiplexing_del_nts(iop, ev);
        }
        if (fd_event_datas_.count(std::make_tuple(iop->fd(), ev)))
        {
            fd_remove_nts(iop, ev);
        }
    }
}

void event_loop::loop_once(int timeout)
{
    auto fd_events = fd_io_multiplexing_wait_ts(timeout);
    for (const auto &fd_ev_tp : fd_events)
    {
        LOG_DEBUG_FMT("Triggered fd %d %s event", std::get<0>(fd_ev_tp), fd_event_debug[std::get<1>(fd_ev_tp)]);
    }
    std::priority_queue<std::tuple<priority, std::shared_ptr<nio>, std::shared_ptr<fd_event_handler>>> fd_callbacks;
    {
        std::unique_lock<std::mutex> lock(lock_);
        for (const auto &fd_ev_tp : fd_events)
        {
            int fd = std::get<0>(fd_ev_tp);
            fd_event ev = std::get<1>(fd_ev_tp);
            if (fd_event_datas_.count(fd_ev_tp))
            {
                if (fd_event_masks_.count(fd) && static_cast<bool>(fd_event_masks_[fd]&ev))
                {
                    const auto &value = fd_event_datas_[fd_ev_tp];
                    fd_callbacks.emplace(std::get<0>(value), std::get<1>(value), std::get<2>(value));
                }
                else
                {
                    LOG_WARNING_FMT("Trying to proceed fd %d %s event but it's not activate", fd, fd_event_debug[ev]);
                }
            }
            else
            {
                LOG_WARNING_FMT("Trying to proceed fd %d %s event but callback data not found", fd, fd_event_debug[ev]);
            }
        }
    }

    while (fd_callbacks.size())
    {
        auto ev = fd_callbacks.top();
        fd_callbacks.pop();
        (*std::get<2>(ev))(std::get<1>(ev));
    }
}

void event_loop::stop_loop_once()
{
    auto iopps = nio_factory::get_pipes();
    iopps[1]->set_evlp(*this);
    LOG_DEBUG_FMT("Use fd %d fd_writable event for event loop stop", iopps[1]->fd());
    fd_event_handler handler = [](const std::shared_ptr<nio> &iop)
    {
        iop->evlp().fd_remove_and_deactivate(iop, fd_event::fd_writable);
        LOG_DEBUG_FMT("Remove fd %d fd_writable event for event loop stop", iop->fd());
        std::unique_lock<std::mutex> lock(iop->evlp().lock_);
        iop->evlp().stop_ = true;
        iop->evlp().cond_.notify_all();
    };
    this->fd_register_and_activate(std::dynamic_pointer_cast<nio>(iopps[1]), fd_event::fd_writable,
        handler, priority::p6);

    std::unique_lock<std::mutex> lock(lock_);
    cond_.wait(lock, [this] { return this->stop_; });
}

void event_loop::loop_forever(int timeout)
{
    stop_ = false;
    while(!stop_)
    {
        loop_once(timeout);
    }
}

void event_loop::stop_loop_forever()
{
    stop_loop_once();
}



void event_loop::fd_register_nts(const std::shared_ptr<nio> &iop, fd_event ev_type,
    const fd_event_handler &handler, priority prio)
{
    iop->set_evlp(*this);
    auto fd_ev_tp = std::make_tuple(iop->fd(), ev_type);
    fd_event_datas_.emplace(fd_ev_tp, std::make_tuple(prio, iop, std::make_shared<fd_event_handler>(handler)));
}

void event_loop::fd_remove_nts(const std::shared_ptr<nio> &iop, fd_event ev_type)
{
    auto fd_ev_tp = std::make_tuple(iop->fd(), ev_type);
    fd_event_datas_.erase(fd_ev_tp);
}

}   // namespace cppev
