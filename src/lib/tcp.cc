#include "cppev/tcp.h"

#include "cppev/logger.h"

namespace cppev
{

namespace reactor
{

tp_shared_data::tp_shared_data(void *external_data_ptr)
    : on_accept(idle_handler),
      on_connect(idle_handler),
      on_read_complete(idle_handler),
      on_write_complete(idle_handler),
      on_closed(idle_handler),
      external_data_ptr(external_data_ptr)
{
}

tp_shared_data::~tp_shared_data() = default;

event_loop *tp_shared_data::random_get_evlp()
{
    std::random_device rd;
    std::default_random_engine rde(rd());
    std::uniform_int_distribution<int> dist(0, evls.size() - 1);
    return evls[dist(rde)];
}

event_loop *tp_shared_data::minloads_get_evlp()
{
    int minloads = INT32_MAX;
    event_loop *minloads_evlp = nullptr;
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

void *tp_shared_data::external_data() noexcept
{
    return external_data_ptr;
}

const void *tp_shared_data::external_data() const noexcept
{
    return external_data_ptr;
}

void async_write(const std::shared_ptr<socktcp> &iopt)
{
    tp_shared_data *dp =
        reinterpret_cast<tp_shared_data *>(iopt->evlp().data());
    iopt->write_all();
    if (0 == iopt->wbuffer().size())
    {
        dp->on_write_complete(iopt);
    }
    else
    {
        if (iopt->eop() || iopt->is_reset())
        {
            if (!iopt->is_closed())
            {
                dp->on_closed(iopt);
                std::shared_ptr<io> iop = std::static_pointer_cast<io>(iopt);
                iopt->evlp().fd_clean(iop);
                iopt->close();
            }
        }
        else
        {
            std::shared_ptr<io> iop = std::static_pointer_cast<io>(iopt);
            iopt->evlp().fd_activate(iop, fd_event::fd_writable);
        }
    }
}

void safely_close(const std::shared_ptr<socktcp> &iopt)
{
    std::shared_ptr<io> iop = std::static_pointer_cast<io>(iopt);
    // epoll/kqueue will remove fd when it's closed
    iopt->evlp().fd_clean(iop);
    iopt->close();
}

void *external_data(const std::shared_ptr<socktcp> &iopt)
{
    return (reinterpret_cast<tp_shared_data *>(iopt->evlp().data()))
        ->external_data();
}

size_t host_hash::operator()(
    const std::tuple<std::string, int, family> &h) const
{
    size_t ret = 0;
    ret += std::hash<std::string>()(std::get<0>(h));
    ret += static_cast<size_t>(std::get<1>(h)) * 100;
    ret += static_cast<size_t>(std::get<2>(h)) * 10;
    return ret;
}

const tcp_event_handler tp_shared_data::idle_handler =
    [](const std::shared_ptr<socktcp> &) -> void
{
};

iohandler::iohandler(tp_shared_data *data)
    : evlp_(reinterpret_cast<void *>(data), reinterpret_cast<void *>(this))
{
}

iohandler::~iohandler() = default;

void iohandler::on_readable(const std::shared_ptr<io> &iop)
{
    std::shared_ptr<socktcp> iopt = std::dynamic_pointer_cast<socktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }
    tp_shared_data *dp = reinterpret_cast<tp_shared_data *>(iop->evlp().data());
    iopt->read_all();
    dp->on_read_complete(iopt);
    if (0 == iopt->rbuffer().size())
    {
        iopt->rbuffer().clear();
    }
    else if ((iopt->rbuffer().capacity() >> 1) < iopt->rbuffer().waste())
    {
        iopt->rbuffer().tiny();
    }
    if ((iopt->eof() || iopt->is_reset()) && (!iopt->is_closed()))
    {
        dp->on_closed(iopt);
        iopt->evlp().fd_clean(iop);
        iopt->close();
    }
}

void iohandler::on_writable(const std::shared_ptr<io> &iop)
{
    std::shared_ptr<socktcp> iopt = std::dynamic_pointer_cast<socktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }
    tp_shared_data *dp = reinterpret_cast<tp_shared_data *>(iop->evlp().data());
    iopt->write_all();
    if (0 == iopt->wbuffer().size())
    {
        iopt->wbuffer().clear();
        iopt->evlp().fd_deactivate(iop, fd_event::fd_writable);
        dp->on_write_complete(iopt);
    }
    else if ((iopt->wbuffer().capacity() >> 1) < iopt->wbuffer().waste())
    {
        iopt->wbuffer().tiny();
    }
    if ((iopt->eop() || iopt->is_reset()) && (!iopt->is_closed()))
    {
        dp->on_closed(iopt);
        iopt->evlp().fd_clean(iop);
        iopt->close();
    }
}

void iohandler::on_acpt_writable(const std::shared_ptr<io> &iop)
{
    std::shared_ptr<socktcp> iopt = std::dynamic_pointer_cast<socktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }
    tp_shared_data *dp =
        reinterpret_cast<tp_shared_data *>(iopt->evlp().data());
    iopt->evlp().fd_remove_and_deactivate(iop, fd_event::fd_writable);
    // The sequence CANNOT be changed, since on_accept may call async_write
    iopt->evlp().fd_register(iop, fd_event::fd_writable,
                             iohandler::on_writable);
    dp->on_accept(iopt);
    iopt->evlp().fd_register_and_activate(iop, fd_event::fd_readable,
                                          iohandler::on_readable);
    LOG_INFO_FMT("Connected socket %d initialized", iop->fd());
}

void iohandler::on_cont_writable(const std::shared_ptr<io> &iop)
{
    std::shared_ptr<socktcp> iopt = std::dynamic_pointer_cast<socktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }

    iohandler *pseudo_this =
        reinterpret_cast<iohandler *>(iopt->evlp().owner());
    iopt->evlp().fd_remove_and_deactivate(iop, fd_event::fd_writable);

    if (!iopt->check_connect())
    {
        std::tuple<std::string, int, family> h = iopt->connpeer();
        LOG_ERROR_FMT("Connect %s %d failed when checking writable",
                      std::get<0>(h).c_str(), std::get<1>(h));
        pseudo_this->failures_[h] += 1;
        iopt->evlp().fd_clean(iop);
        iopt->close();
        return;
    }
    tp_shared_data *dp = reinterpret_cast<tp_shared_data *>(iop->evlp().data());
    // The sequence CANNOT be changed since on_connect may call aysnc_write
    iopt->evlp().fd_register(iop, fd_event::fd_writable,
                             iohandler::on_writable);
    dp->on_connect(iopt);
    iopt->evlp().fd_register_and_activate(iop, fd_event::fd_readable,
                                          iohandler::on_readable);
    LOG_INFO_FMT("Connected socket %d initialized", iop->fd());
}

void iohandler::run_impl()
{
    LOG_INFO << "Thread iohandler starting";
    evlp_.loop_forever();
    LOG_INFO << "Thread iohandler ending";
}

void iohandler::shutdown()
{
    evlp_.stop_loop_forever();
}

acceptor::acceptor(tp_shared_data *data)
    : evlp_(reinterpret_cast<void *>(data), reinterpret_cast<void *>(this))
{
}

acceptor::~acceptor() = default;

void acceptor::listen(int port, family f, const char *ip)
{
    std::shared_ptr<socktcp> sock = io_factory::get_socktcp(f);
    sock->bind(ip, port);
    sock->listen();
    socks_.push_back(sock);
    LOG_INFO_FMT("Listening socket %d working in port %d", sock->fd(), port);
}

void acceptor::listen_unix(const std::string &path, bool remove)
{
    std::shared_ptr<socktcp> sock = io_factory::get_socktcp(family::local);
    sock->bind_unix(path, remove);
    sock->listen();
    socks_.push_back(sock);
    LOG_INFO_FMT("Listening socket %d working in path %s", sock->fd(),
                 path.c_str());
}

void acceptor::on_acpt_readable(const std::shared_ptr<io> &iop)
{
    std::shared_ptr<socktcp> iopt = std::dynamic_pointer_cast<socktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }
    std::vector<std::shared_ptr<socktcp>> conns = iopt->accept();
    tp_shared_data *dp =
        reinterpret_cast<tp_shared_data *>(iopt->evlp().data());

    for (auto &conn : conns)
    {
        LOG_INFO_FMT("Listening socket %d accepted new socket %d", iopt->fd(),
                     conn->fd());
        dp->minloads_get_evlp()->fd_register_and_activate(
            std::static_pointer_cast<io>(conn), fd_event::fd_writable,
            iohandler::on_acpt_writable);
    }
}

void acceptor::run_impl()
{
    LOG_INFO << "Thread acceptor starting";
    for (auto &sock : socks_)
    {
        evlp_.fd_register_and_activate(std::static_pointer_cast<io>(sock),
                                       fd_event::fd_readable,
                                       acceptor::on_acpt_readable);
    }
    evlp_.loop_forever();
    LOG_INFO << "Thread acceptor ending";
}

void acceptor::shutdown()
{
    evlp_.stop_loop_forever();
}

connector::connector(tp_shared_data *data)
    : evlp_(reinterpret_cast<void *>(data), reinterpret_cast<void *>(this))
{
    auto pipes = io_factory::get_pipes();
    rdp_ = pipes[0];
    wrp_ = pipes[1];
}

connector::~connector() = default;

void connector::add(const std::string &ip, int port, family f, int t)
{
    if (t == 0)
    {
        return;
    }
    auto h = std::make_tuple(ip, port, f);

    {
        std::unique_lock<std::mutex> _(lock_);
        if (hosts_.count(h))
        {
            hosts_[h] += t;
        }
        else
        {
            hosts_[h] = t;
        }
    }

    wrp_->wbuffer().put_string("0");
    wrp_->write_all(1);
}

void connector::add_unix(const std::string &path, int t)
{
    add(path, 0, family::local, t);
}

void connector::on_pipe_readable(const std::shared_ptr<io> &iop)
{
    stream *iops = dynamic_cast<stream *>(iop.get());
    if (iops == nullptr)
    {
        throw_logic_error("dynamic_cast error");
    }
    tp_shared_data *dp =
        reinterpret_cast<tp_shared_data *>(iops->evlp().data());
    iops->read_all(1);

    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash>
        hosts;
    connector *pseudo_this =
        reinterpret_cast<connector *>(iops->evlp().owner());
    {
        std::unique_lock<std::mutex> _(pseudo_this->lock_);
        pseudo_this->hosts_.swap(hosts);
    }

    for (auto iter = hosts.begin(); iter != hosts.end();)
    {
        for (int i = 0; i < iter->second; ++i)
        {
            std::shared_ptr<socktcp> sock =
                io_factory::get_socktcp(std::get<2>(iter->first));
            bool succeed;
            if (std::get<2>(iter->first) == family::local)
            {
                succeed = sock->connect_unix(std::get<0>(iter->first));
            }
            else
            {
                succeed = sock->connect(std::get<0>(iter->first),
                                        std::get<1>(iter->first));
            }
            if (succeed)
            {
                dp->minloads_get_evlp()->fd_register_and_activate(
                    std::static_pointer_cast<io>(sock), fd_event::fd_writable,
                    iohandler::on_cont_writable);
            }
            else
            {
                std::error_code err_code(errno, std::system_category());
                if (std::get<2>(iter->first) == family::local)
                {
                    LOG_ERROR_FMT(
                        "Connect %s failed with syscall errno %d : %s",
                        std::get<0>(iter->first).c_str(), err_code.value(),
                        err_code.message().c_str());
                }
                else
                {
                    LOG_ERROR_FMT(
                        "Connect %s %d failed with syscall errno %d : %s",
                        std::get<0>(iter->first).c_str(),
                        std::get<1>(iter->first), err_code.value(),
                        err_code.message().c_str());
                }
                pseudo_this->failures_[iter->first] += 1;
            }
        }
        iter = hosts.erase(iter);
    }
}

void connector::run_impl()
{
    LOG_INFO << "Thread connector starting";
    evlp_.fd_register_and_activate(std::static_pointer_cast<io>(rdp_),
                                   fd_event::fd_readable,
                                   connector::on_pipe_readable);
    evlp_.loop_forever();
    LOG_INFO << "Thread connector ending";
}

void connector::shutdown()
{
    evlp_.stop_loop_forever();
}

tcp_server::tcp_server(int iohandler_num, bool single_acceptor,
                       void *external_data)
    : data_(external_data),
      single_acceptor_(single_acceptor),
      tp_(iohandler_num, &data_)
{
    for (int i = 0; i < tp_.size(); ++i)
    {
        data_.evls.push_back(&(tp_[i].evlp_));
    }
}

tcp_server::~tcp_server() = default;

void tcp_server::set_on_accept(const tcp_event_handler &handler)
{
    data_.on_accept = handler;
}

void tcp_server::set_on_read_complete(const tcp_event_handler &handler)
{
    data_.on_read_complete = handler;
}

void tcp_server::set_on_write_complete(const tcp_event_handler &handler)
{
    data_.on_write_complete = handler;
}

void tcp_server::set_on_closed(const tcp_event_handler &handler)
{
    data_.on_closed = handler;
}

void tcp_server::listen(int port, family f, const char *ip)
{
    if ((!single_acceptor_) || acpts_.empty())
    {
        acpts_.push_back(std::make_unique<acceptor>(&data_));
    }
    acpts_.back()->listen(port, f, ip);
}

void tcp_server::listen_unix(const std::string &path, bool remove)
{
    if ((!single_acceptor_) || acpts_.empty())
    {
        acpts_.push_back(std::make_unique<acceptor>(&data_));
    }
    acpts_.back()->listen_unix(path, remove);
}

void tcp_server::run()
{
    ignore_signal(SIGPIPE);
    tp_.run();
    for (auto &acpt : acpts_)
    {
        acpt->run();
    }
}

void tcp_server::shutdown()
{
    for (auto &acpt : acpts_)
    {
        acpt->shutdown();
    }
    for (auto &acpt : acpts_)
    {
        acpt->join();
    }

    for (int i = 0; i < tp_.size(); ++i)
    {
        tp_[i].shutdown();
    }
    for (int i = 0; i < tp_.size(); ++i)
    {
        tp_[i].join();
    }
}

tcp_client::tcp_client(int iohandler_num, int connector_num,
                       void *external_data)
    : data_(external_data), tp_(iohandler_num, &data_)
{
    for (int i = 0; i < tp_.size(); ++i)
    {
        data_.evls.push_back(&(tp_[i].evlp_));
    }
    for (int i = 0; i < connector_num; ++i)
    {
        conts_.push_back(std::make_unique<connector>(&data_));
    }
}

tcp_client::~tcp_client() = default;

void tcp_client::set_on_connect(const tcp_event_handler &handler)
{
    data_.on_connect = handler;
}

void tcp_client::set_on_read_complete(const tcp_event_handler &handler)
{
    data_.on_read_complete = handler;
}

void tcp_client::set_on_write_complete(const tcp_event_handler &handler)
{
    data_.on_write_complete = handler;
}

void tcp_client::set_on_closed(const tcp_event_handler &handler)
{
    data_.on_closed = handler;
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
    ignore_signal(SIGPIPE);
    tp_.run();
    for (auto &cont : conts_)
    {
        cont->run();
    }
}

void tcp_client::shutdown()
{
    for (auto &cont : conts_)
    {
        cont->shutdown();
    }
    for (auto &cont : conts_)
    {
        cont->join();
    }

    for (int i = 0; i < tp_.size(); ++i)
    {
        tp_[i].shutdown();
    }
    for (int i = 0; i < tp_.size(); ++i)
    {
        tp_[i].join();
    }
}

}  // namespace reactor

}  // namespace cppev
