#include "cppev/tcp.h"

namespace cppev
{

namespace tcp
{

// Idle function for callback
tcp_event_cb idle_handler = [](std::shared_ptr<nsocktcp>) -> void {};

tp_shared_data::tp_shared_data()
:
    on_accept(idle_handler),
    on_connect(idle_handler),
    on_read_complete(idle_handler),
    on_write_complete(idle_handler),
    on_closed(idle_handler)
{}

event_loop *tp_shared_data::random_get_evlp()
{
    std::random_device rd;
    std::default_random_engine rde(rd());
    std::uniform_int_distribution<int> dist(0, evls.size()-1);
    return evls[dist(rde)];
}

event_loop *tp_shared_data::minloads_get_evlp()
{
    int minloads = INT32_MAX;
    event_loop *minloads_evp;
    for (auto evp : evls)
    {
        // This is not thread safe but it's okay
        if (evp->fd_loads() < minloads)
        {
            minloads_evp = evp;
            minloads = evp->fd_loads();
        }
    }
    return minloads_evp;
}

void async_write(std::shared_ptr<nsocktcp> iopt)
{
    tp_shared_data *dp = static_cast<tp_shared_data *>(iopt->evlp()->data());
    iopt->write_all(sysconfig::buffer_io_step);
    if (0 == iopt->wbuf()->size())
    {
        dp->on_write_complete(iopt);
    }
    else
    {
        std::shared_ptr<nio> iop = std::dynamic_pointer_cast<nio>(iopt);
        if (iop == nullptr)
        {
            throw_logic_error("dynamic_cast error");
        }
        iopt->evlp()->fd_remove(iop, false);
        iopt->evlp()->fd_register(iop, fd_event::fd_writable);
    }
}

void safely_close(std::shared_ptr<nsocktcp> iopt)
{
    std::shared_ptr<cppev::nio> iop = std::dynamic_pointer_cast<cppev::nio>(iopt);
    try
    {
        iopt->evlp()->fd_remove(iop, true);
    }
    catch(...) {}

    iopt->close();
}

void iohandler::on_readable(std::shared_ptr<nio> iop)
{
    std::shared_ptr<nsocktcp> iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_cast error");
    }
    tp_shared_data *dp = static_cast<tp_shared_data *>(iop->evlp()->data());
    iopt->read_all(sysconfig::buffer_io_step);
    dp->on_read_complete(iopt);
    iopt->rbuf()->clear();
    if (iopt->eof() || iopt->is_reset())
    {
        dp->on_closed(iopt);
        iop->evlp()->fd_remove(iop, true);
    }
}

void iohandler::on_writable(std::shared_ptr<nio> iop)
{
    std::shared_ptr<nsocktcp> iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
    if (iopt.get() == nullptr)
    {
        throw_logic_error("dynamic_cast error");
    }
    tp_shared_data *dp = static_cast<tp_shared_data *>(iop->evlp()->data());
    iopt->write_all(sysconfig::buffer_io_step);
    if (0 == iopt->wbuf()->size())
    {
        iop->evlp()->fd_remove(iop, false);
        dp->on_write_complete(iopt);
        if (!iop->is_closed())
        {
            iop->evlp()->fd_register(iop, fd_event::fd_readable);
        }
    }
    if (iopt->eop() || iopt->is_reset())
    {
        dp->on_closed(iopt);
        iop->evlp()->fd_remove(iop, true);
    }
}

void acceptor::listen(int port, family f, const char *ip)
{
    sock_ = nio_factory::get_nsocktcp(f);
    sock_->listen(port, ip);
}

void acceptor::on_readable(std::shared_ptr<nio> iop)
{
    std::shared_ptr<nsocktcp> iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
    if (iopt.get() == nullptr)
    {
        throw_logic_error("dynamic_cast error");
    }
    std::vector<std::shared_ptr<nsocktcp> > conns = iopt->accept();
    tp_shared_data *d = static_cast<tp_shared_data *>(iop->evlp()->data());

    for (auto &p : conns)
    {
        log::info << "new fd " << p->fd() << " accepted" << log::endl;
        event_loop *io_evlp = d->minloads_get_evlp();
        io_evlp->fd_register(std::dynamic_pointer_cast<nio>(p),
            fd_event::fd_writable, acceptor::on_writable, true);
    }
}

void acceptor::on_writable(std::shared_ptr<nio> iop)
{
    std::shared_ptr<nsocktcp> iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
    if (iopt.get() == nullptr)
    {
        throw_logic_error("dynamic_cast error");
    }
    tp_shared_data *d = static_cast<tp_shared_data *>(iop->evlp()->data());
    iop->evlp()->fd_remove(iop, true);
    // The sequence CANNOT be changed, since on_accept may call async_write
    iop->evlp()->fd_register(iop, fd_event::fd_writable, iohandler::on_writable, false);
    d->on_accept(iopt);
    iop->evlp()->fd_register(iop, fd_event::fd_readable, iohandler::on_readable, true);
}

void acceptor::run_impl()
{
    evp_->fd_register(std::dynamic_pointer_cast<nio>(sock_),
        fd_event::fd_readable, acceptor::on_readable, true);
    evp_->loop();
}

void connector::add(std::string ip, int port, family f, int t)
{
    tp_shared_data *d = static_cast<tp_shared_data *>(evp_->data());
    auto h = std::make_tuple<>(ip, port, f);
    if (d->hosts.count(h))
    {
        d->hosts[h] += t;
    }
    else
    {
        d->hosts[h] = t;
    }
    wrp_->wbuf()->put("0");
    wrp_->write_all(1);
}

void connector::on_readable(std::shared_ptr<nio> iop)
{
    nstream *iops = dynamic_cast<nstream *>(iop.get());
    if (iops == nullptr)
    {
        throw_logic_error("dynamic_cast error");
    }
    tp_shared_data *d = static_cast<tp_shared_data *>(iop->evlp()->data());
    iops->read_all(1);
    for (auto iter = d->hosts.begin(); iter != d->hosts.end(); )
    {
        for (int i = 0; i < iter->second; ++i)
        {
            std::shared_ptr<nsocktcp> sock =
                nio_factory::get_nsocktcp(std::get<2>(iter->first));
            sock->connect(std::get<0>(iter->first), std::get<1>(iter->first));
            event_loop *io_evlp = d->minloads_get_evlp();
            io_evlp->fd_register(std::dynamic_pointer_cast<nio>(sock),
                fd_event::fd_writable, connector::on_writable, true);
        }
        iter = d->hosts.erase(iter);
    }
}

void connector::on_writable(std::shared_ptr<nio> iop)
{
    std::shared_ptr<nsocktcp> iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
    if (iopt.get() == nullptr)
    {
        throw_logic_error("dynamic_cast error");
    }
    tp_shared_data *d = static_cast<tp_shared_data *>(iop->evlp()->data());
    iop->evlp()->fd_remove(iop, true);      // remove previous callback
    if (!iopt->check_connect())
    {
        std::tuple<std::string, int, family> h = iopt->connpeer();
        log::error << "connect failed with " << std::get<0>(h)
            << " " << std::get<1>(h) << log::endl;
        if (d->failures.count(h))
        {
            d->failures[h] += 1;
        }
        else
        {
            d->failures[h] = 1;
        }
        return;
    }
    // The sequence CANNOT be changed since on_connect may call aysnc_write
    iop->evlp()->fd_register(iop, fd_event::fd_writable, iohandler::on_writable, false);
    d->on_connect(iopt);
    iop->evlp()->fd_register(iop, fd_event::fd_readable, iohandler::on_readable, true);
}

void connector::run_impl()
{
    evp_->fd_register(std::dynamic_pointer_cast<nio>(rdp_),
        fd_event::fd_readable, connector::on_readable, true);
    evp_->loop();
}

tcp_server::tcp_server(int thr_num)
{
    data_ = std::shared_ptr<tp_shared_data>(new tp_shared_data);
    tp_ = std::shared_ptr<thread_pool<iohandler, tp_shared_data *> >
        (new thread_pool<iohandler, tp_shared_data *>(thr_num, data_.get()));
    for (int i = 0; i < tp_->size(); ++i)
    {
        data_->evls.push_back((*tp_)[i]->evp_.get());
    }
    acpt_ = std::shared_ptr<acceptor>(new acceptor(data_.get()));
}

void tcp_server::run()
{
    utils::ignore_signal(SIGPIPE);
    tp_->run();
    acpt_->run();
    tp_->join();
    acpt_->join();
}

tcp_client::tcp_client(int thr_num)
{
    data_ = std::shared_ptr<tp_shared_data>(new tp_shared_data);
    tp_ = std::shared_ptr<thread_pool<iohandler, tp_shared_data *> >
        (new thread_pool<iohandler, tp_shared_data *>(thr_num, data_.get()));
    for (int i = 0; i < tp_->size(); ++i)
    {
        data_->evls.push_back((*tp_)[i]->evp_.get());
    }
    cont_ = std::shared_ptr<connector>(new connector(data_.get()));
}

void tcp_client::run()
{
    utils::ignore_signal(SIGPIPE);
    tp_->run();
    cont_->run();
    tp_->join();
    cont_->join();
}

}   // namespace tcp

}   // namespace cppev
