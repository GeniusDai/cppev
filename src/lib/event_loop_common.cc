#include "cppev/event_loop.h"

namespace cppev
{

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
    return fds_.size();
}

void event_loop::stop_loop_once()
{
    auto iopps = nio_factory::get_pipes();
    iopps[0]->set_evlp(*this);
    fd_event_handler handler = [](const std::shared_ptr<nio> &iop)
    {
        iop->evlp().fd_remove(iop);
    };
    this->fd_register(std::dynamic_pointer_cast<nio>(iopps[0]), fd_event::fd_readable,
        handler, true, priority::p6);
}

void event_loop::loop_forever(int timeout)
{
    while(!stop_)
    {
        loop_once(timeout);
    }
}

// Stop loop infinitely
void event_loop::stop_loop_forever()
{
    stop_ = true;
    stop_loop_once();
}

}   // namespace cppev
