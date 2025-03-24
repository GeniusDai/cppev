#include "cppev/event_loop.h"

namespace cppev
{

fd_event operator&(fd_event lhs, fd_event rhs)
{
    return static_cast<fd_event>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

fd_event operator|(fd_event lhs, fd_event rhs)
{
    return static_cast<fd_event>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

fd_event operator^(fd_event lhs, fd_event rhs)
{
    return static_cast<fd_event>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

void operator&=(fd_event &lhs, fd_event rhs)
{
    lhs = static_cast<fd_event>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

void operator|=(fd_event &lhs, fd_event rhs)
{
    lhs = static_cast<fd_event>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

void operator^=(fd_event &lhs, fd_event rhs)
{
    lhs = static_cast<fd_event>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

const std::unordered_map<fd_event, const char *> fd_event_to_string = {
    {fd_event::fd_readable, "fd_readable"},
    {fd_event::fd_writable, "fd_writable"},
};

std::size_t fd_event_hash::operator()(
    const std::tuple<int, fd_event> &ev) const noexcept
{
    return std::hash<std::size_t>()((std::get<0>(ev) << 2) +
                                    static_cast<int>(std::get<1>(ev)));
}

const fd_event_mode event_loop::fd_event_mode_default_ =
    fd_event_mode::level_trigger;

event_loop::event_loop(void *data, void *owner)
    : data_(data), owner_(owner), stop_(false)
{
    fd_io_multiplexing_create_nts();
}

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

void event_loop::fd_set_mode(const std::shared_ptr<io> &iop,
                             fd_event_mode ev_mode)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_event_modes_[iop->fd()] = ev_mode;
}

void event_loop::fd_register(const std::shared_ptr<io> &iop, fd_event ev_type,
                             const fd_event_handler &handler, priority prio)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_register_nts(iop, ev_type, handler, prio);
}

void event_loop::fd_activate(const std::shared_ptr<io> &iop, fd_event ev_type)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_io_multiplexing_add_nts(iop, ev_type);
}

void event_loop::fd_register_and_activate(const std::shared_ptr<io> &iop,
                                          fd_event ev_type,
                                          const fd_event_handler &handler,
                                          priority prio)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_register_nts(iop, ev_type, handler, prio);
    fd_io_multiplexing_add_nts(iop, ev_type);
}

void event_loop::fd_remove(const std::shared_ptr<io> &iop, fd_event ev_type)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_remove_nts(iop, ev_type);
}

void event_loop::fd_deactivate(const std::shared_ptr<io> &iop, fd_event ev_type)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_io_multiplexing_del_nts(iop, ev_type);
}

void event_loop::fd_remove_and_deactivate(const std::shared_ptr<io> &iop,
                                          fd_event ev_type)
{
    std::unique_lock<std::mutex> lock(lock_);
    fd_io_multiplexing_del_nts(iop, ev_type);
    fd_remove_nts(iop, ev_type);
}

void event_loop::fd_clean(const std::shared_ptr<io> &iop)
{
    std::unique_lock<std::mutex> lock(lock_);
    for (auto ev : {fd_event::fd_readable, fd_event::fd_writable})
    {
        if (fd_event_masks_.count(iop->fd()) &&
            static_cast<bool>(ev & fd_event_masks_[iop->fd()]))
        {
            fd_io_multiplexing_del_nts(iop, ev);
        }
        if (fd_event_datas_.count(std::make_tuple(iop->fd(), ev)))
        {
            fd_remove_nts(iop, ev);
        }
    }
    fd_event_modes_.erase(iop->fd());
    iop->set_evlp(nullptr);
}

void event_loop::loop_once(int timeout)
{
    auto fd_events = fd_io_multiplexing_wait_ts(timeout);
    for (const auto &fd_ev_tp : fd_events)
    {
        LOG_DEBUG_FMT("About to trigger fd %d %s event", std::get<0>(fd_ev_tp),
                      fd_event_to_string.at(std::get<1>(fd_ev_tp)));
    }
    std::priority_queue<std::tuple<priority, std::shared_ptr<io>,
                                   std::shared_ptr<fd_event_handler>>>
        fd_callbacks;
    {
        std::unique_lock<std::mutex> lock(lock_);
        for (const auto &fd_ev_tp : fd_events)
        {
            int fd = std::get<0>(fd_ev_tp);
            fd_event ev = std::get<1>(fd_ev_tp);
            if (fd_event_datas_.count(fd_ev_tp))
            {
                if (fd_event_masks_.count(fd) &&
                    static_cast<bool>(fd_event_masks_[fd] & ev))
                {
                    const auto &value = fd_event_datas_[fd_ev_tp];
                    fd_callbacks.emplace(std::get<0>(value), std::get<1>(value),
                                         std::get<2>(value));
                }
                else
                {
                    LOG_WARNING_FMT(
                        "Trying to proceed fd %d %s event but it's not "
                        "activate",
                        fd, fd_event_to_string.at(ev));
                }
            }
            else
            {
                LOG_WARNING_FMT(
                    "Trying to proceed fd %d %s event but callback data not "
                    "found",
                    fd, fd_event_to_string.at(ev));
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

bool event_loop::stop_loop_ts_wl(waiter_type waiter)
{
    auto iopps = io_factory::get_pipes();
    iopps[1]->set_evlp(this);
    LOG_DEBUG_FMT("Use fd %d fd_writable event for event loop stop",
                  iopps[1]->fd());
    fd_event_handler handler = [](const std::shared_ptr<io> &iop)
    {
        event_loop &evlp = iop->evlp();
        evlp.fd_remove_and_deactivate(iop, fd_event::fd_writable);
        LOG_DEBUG_FMT("Remove fd %d fd_writable event for event loop stop",
                      iop->fd());
        {
            std::unique_lock<std::mutex> lock(evlp.lock_);
            evlp.stop_ = true;
            evlp.cond_.notify_all();
        }
    };
    this->fd_register_and_activate(std::dynamic_pointer_cast<io>(iopps[1]),
                                   fd_event::fd_writable, handler,
                                   priority::lowest);

    std::unique_lock<std::mutex> lock(lock_);
    return waiter(lock);
}

void event_loop::stop_loop()
{
    waiter_type waiter = [this](std::unique_lock<std::mutex> &lock) -> bool
    {
        this->cond_.wait(lock, [this] { return this->stop_; });
        return true;
    };
    stop_loop_ts_wl(std::move(waiter));
}

bool event_loop::stop_loop(int timeout)
{
    waiter_type waiter = [this,
                          timeout](std::unique_lock<std::mutex> &lock) -> bool
    {
        return this->cond_.wait_for(lock, std::chrono::milliseconds(timeout),
                                    [this] { return this->stop_; });
    };
    return stop_loop_ts_wl(std::move(waiter));
}

void event_loop::loop_forever(int timeout)
{
    stop_ = false;
    while (!stop_)
    {
        loop_once(timeout);
    }
}

void event_loop::fd_register_nts(const std::shared_ptr<io> &iop,
                                 fd_event ev_type,
                                 const fd_event_handler &handler, priority prio)
{
    iop->set_evlp(this);
    auto fd_ev_tp = std::make_tuple(iop->fd(), ev_type);
    fd_event_datas_.emplace(
        fd_ev_tp, std::make_tuple(prio, iop,
                                  std::make_shared<fd_event_handler>(handler)));
    if (!fd_event_modes_.count(iop->fd()))
    {
        fd_event_modes_[iop->fd()] = fd_event_mode_default_;
    }
}

void event_loop::fd_remove_nts(const std::shared_ptr<io> &iop, fd_event ev_type)
{
    auto fd_ev_tp = std::make_tuple(iop->fd(), ev_type);
    fd_event_datas_.erase(fd_ev_tp);
}

}  // namespace cppev
