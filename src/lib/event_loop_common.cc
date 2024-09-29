#include "cppev/event_loop.h"

namespace cppev
{

event_loop::~event_loop() noexcept
{
    close(ev_fd_);
}

int event_loop::ev_fd() const noexcept
{
    return ev_fd_;
}

void *event_loop::data() const noexcept
{
    return data_;
}

void *event_loop::back() const noexcept
{
    return back_;
}

int event_loop::ev_loads() const noexcept
{
    return fds_.size();
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
    auto iopps = nio_factory::get_pipes();
    iopps[0]->set_evlp(*this);
    fd_event_handler handler = [](const std::shared_ptr<nio> &iop)
    {
        iop->evlp().fd_remove(iop);
    };
    this->fd_register(std::dynamic_pointer_cast<nio>(iopps[0]), fd_event::fd_readable,
        handler, true, priority::p6);
}

}   // namespace cppev
