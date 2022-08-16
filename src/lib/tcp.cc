#include "cppev/tcp.h"

namespace cppev
{

namespace reactor
{

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
    event_loop *minloads_evlp;
    for (auto evlp : evls)
    {
        // This is not thread safe but it's okay
        if (evlp->ev_loads() < minloads)
        {
            minloads_evlp = evlp;
            minloads = evlp->ev_loads();
        }
    }
    return minloads_evlp;
}


void async_write(const std::shared_ptr<nsocktcp> &iopt)
{
    tp_shared_data *dp = reinterpret_cast<tp_shared_data *>(iopt->evlp()->data());
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
            throw_logic_error("dynamic_pointer_cast error");
        }
        iopt->evlp()->fd_remove(iop, false);
        iopt->evlp()->fd_register(iop, fd_event::fd_writable);
    }
}

void safely_close(const std::shared_ptr<nsocktcp> &iopt)
{
    std::shared_ptr<nio> iop = std::dynamic_pointer_cast<nio>(iopt);
    // epoll/kqueue will remove fd when it's closed
    iopt->evlp()->fd_remove(iop, true, false);
    iopt->close();
}

void *external_data(const std::shared_ptr<nsocktcp> &iopt)
{
    return (reinterpret_cast<tp_shared_data *>(iopt->evlp()->data()))->external_data();
}

const tcp_event_handler tp_shared_data::idle_handler = [](const std::shared_ptr<nsocktcp> &) -> void {};

void iohandler::on_readable(const std::shared_ptr<nio> &iop)
{
    std::shared_ptr<nsocktcp> iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }
    tp_shared_data *dp = reinterpret_cast<tp_shared_data *>(iop->evlp()->data());
    iopt->read_all(sysconfig::buffer_io_step);
    dp->on_read_complete(iopt);
    iopt->rbuf()->clear();
    if (iopt->eof() || iopt->is_reset())
    {
        dp->on_closed(iopt);
        iopt->evlp()->fd_remove(iop, true);
    }
}

void iohandler::on_writable(const std::shared_ptr<nio> &iop)
{
    std::shared_ptr<nsocktcp> iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }
    tp_shared_data *dp = reinterpret_cast<tp_shared_data *>(iop->evlp()->data());
    iopt->write_all(sysconfig::buffer_io_step);
    if (0 == iopt->wbuf()->size())
    {
        iopt->evlp()->fd_remove(iop, false);
        dp->on_write_complete(iopt);
        if (!iopt->is_closed())
        {
            iopt->evlp()->fd_register(iop, fd_event::fd_readable);
        }
    }
    if (iopt->eop() || iopt->is_reset())
    {
        dp->on_closed(iopt);
        iopt->evlp()->fd_remove(iop, true);
    }
}

void iohandler::on_acpt_writable(const std::shared_ptr<nio> &iop)
{
    std::shared_ptr<nsocktcp> iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }
    tp_shared_data *dp = reinterpret_cast<tp_shared_data *>(iopt->evlp()->data());
    iopt->evlp()->fd_remove(iop, true);
    // The sequence CANNOT be changed, since on_accept may call async_write
    iopt->evlp()->fd_register(iop, fd_event::fd_writable, iohandler::on_writable, false);
    dp->on_accept(iopt);
    iopt->evlp()->fd_register(iop, fd_event::fd_readable, iohandler::on_readable, true);
}

void iohandler::on_cont_writable(const std::shared_ptr<nio> &iop)
{
    std::shared_ptr<nsocktcp> iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }

    iohandler *pseudo_this = reinterpret_cast<iohandler *>(iopt->evlp()->back());
    iopt->evlp()->fd_remove(iop, true);      // remove previous callback

    if (!iopt->check_connect())
    {
        std::tuple<std::string, int, family> h = iopt->connpeer();
        log::error << "connect " << std::get<0>(h) << " " << std::get<1>(h)
            << " failed when checking writable" << log::endl;
        pseudo_this->failures_[h] += 1;
        return;
    }
    tp_shared_data *dp = reinterpret_cast<tp_shared_data *>(iop->evlp()->data());
    // The sequence CANNOT be changed since on_connect may call aysnc_write
    iopt->evlp()->fd_register(iop, fd_event::fd_writable, iohandler::on_writable, false);
    dp->on_connect(iopt);
    iopt->evlp()->fd_register(iop, fd_event::fd_readable, iohandler::on_readable, true);
}

void acceptor::listen(int port, family f, const char *ip)
{
    sock_ = nio_factory::get_nsocktcp(f);
    sock_->listen(port, ip);
    log::info << "fd " << sock_->fd() << " listening in port " << port << log::endl;
}

void acceptor::listen_unix(const std::string &path)
{
    sock_ = nio_factory::get_nsocktcp(family::local);
    sock_->listen_unix(path);
    log::info << "fd " << sock_->fd() << " listening in path " << path << log::endl;
}

void acceptor::on_acpt_readable(const std::shared_ptr<nio> &iop)
{
    std::shared_ptr<nsocktcp> iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }
    std::vector<std::shared_ptr<nsocktcp> > conns = iopt->accept();
    tp_shared_data *dp = reinterpret_cast<tp_shared_data *>(iopt->evlp()->data());

    for (auto &conn : conns)
    {
        log::info << "new fd " << conn->fd() << " accepted by listening socket " << iopt->fd() << log::endl;
        dp->minloads_get_evlp()->fd_register(std::dynamic_pointer_cast<nio>(conn),
            fd_event::fd_writable, iohandler::on_acpt_writable, true);
    }
}

void acceptor::run_impl()
{
    evlp_->fd_register(std::dynamic_pointer_cast<nio>(sock_),
        fd_event::fd_readable, acceptor::on_acpt_readable, true);
    evlp_->loop();
}

void connector::add(const std::string &ip, int port, family f, int t)
{
    if (t == 0)
    {
        return;
    }
    auto h = std::make_tuple<>(ip, port, f);
    if (hosts_.count(h))
    {
        hosts_[h] += t;
    }
    else
    {
        hosts_[h] = t;
    }
    wrp_->wbuf()->put_string("0");
    wrp_->write_all(1);
}

void connector::add_unix(const std::string &path, int t)
{
    add(path, 0, family::local, t);
}

void connector::on_pipe_readable(const std::shared_ptr<nio> &iop)
{
    nstream *iops = dynamic_cast<nstream *>(iop.get());
    if (iops == nullptr)
    {
        throw_logic_error("dynamic_cast error");
    }
    tp_shared_data *dp = reinterpret_cast<tp_shared_data *>(iops->evlp()->data());
    iops->read_all(1);
    connector *pseudo_this = reinterpret_cast<connector *>(iops->evlp()->back());
    for (auto iter = pseudo_this->hosts_.begin(); iter != pseudo_this->hosts_.end(); )
    {
        for (int i = 0; i < iter->second; ++i)
        {
            std::shared_ptr<nsocktcp> sock = nio_factory::get_nsocktcp(std::get<2>(iter->first));
            bool succeed;
            if (std::get<2>(iter->first) == family::local)
            {
                succeed = sock->connect_unix(std::get<0>(iter->first));
            }
            else
            {
                succeed = sock->connect(std::get<0>(iter->first), std::get<1>(iter->first));
            }
            if (succeed)
            {
                dp->minloads_get_evlp()->fd_register(std::dynamic_pointer_cast<nio>(sock),
                    fd_event::fd_writable, iohandler::on_cont_writable, true);
            }
            else
            {
                if (std::get<2>(iter->first) == family::local)
                {
                    log::error << "syscall connect " << std::get<0>(iter->first) <<
                        " failed with errno " << errno << log::endl;
                }
                else
                {
                    log::error << "syscall connect " << std::get<0>(iter->first) << " "
                        << std::get<1>(iter->first) << " failed with errno " << errno << log::endl;
                }
                pseudo_this->failures_[iter->first] += 1;
            }
        }
        iter = pseudo_this->hosts_.erase(iter);
    }
}

void connector::run_impl()
{
    evlp_->fd_register(std::dynamic_pointer_cast<nio>(rdp_),
        fd_event::fd_readable, connector::on_pipe_readable, true);
    evlp_->loop();
}

tcp_server::tcp_server(int thr_num, void *external_data)
{
    data_ = std::make_shared<tp_shared_data>(external_data);
    tp_ = std::make_shared<thread_pool<iohandler, tp_shared_data *> >(thr_num, data_.get());
    for (int i = 0; i < tp_->size(); ++i)
    {
        data_->evls.push_back((*tp_)[i]->evlp_.get());
    }
    data_->reactor_ptr = this;
}

void tcp_server::run()
{
    utils::ignore_signal(SIGPIPE);
    tp_->run();
    for (auto &acpt : acpts_)
    {
        acpt->run();
    }
    tp_->join();
    for (auto &acpt : acpts_)
    {
        acpt->join();
    }
}

tcp_client::tcp_client(int thr_num, int cont_num, void *external_data)
{
    data_ = std::make_shared<tp_shared_data>(external_data);
    tp_ = std::make_shared<thread_pool<iohandler, tp_shared_data *> >(thr_num, data_.get());
    for (int i = 0; i < tp_->size(); ++i)
    {
        data_->evls.push_back((*tp_)[i]->evlp_.get());
    }
    for (int i = 0; i < cont_num; ++i)
    {
        conts_.push_back(std::make_shared<connector>(data_.get()));
    }
    data_->reactor_ptr = this;
}

void tcp_client::add(const std::string &ip, int port, family f, int t)
{
    if (conts_.size() == 1)
    {
        conts_[0]->add(ip, port, f, t);
    }
    else
    {
        int num = t / conts_.size();
        int mod = t % conts_.size();
        for (auto &cont : conts_)
        {
            cont->add(ip, port, f, num);
        }
        conts_[0]->add(ip, port, f, mod);
    }
}

void tcp_client::add_unix(const std::string &path, int t)
{
    if (conts_.size() == 1)
    {
        conts_[0]->add_unix(path, t);
    }
    else
    {
        int num = t / conts_.size();
        int mod = t % conts_.size();
        for (auto &cont : conts_)
        {
            cont->add_unix(path, num);
        }
        conts_[0]->add_unix(path, mod);
    }
}

void tcp_client::run()
{
    utils::ignore_signal(SIGPIPE);
    tp_->run();
    for (auto &cont : conts_)
    {
        cont->run();
    }
    tp_->join();
    for (auto &cont : conts_)
    {
        cont->join();
    }
}

}   // namespace reactor

}   // namespace cppev
