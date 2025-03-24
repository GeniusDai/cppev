#include "cppev/tcp.h"

#include "cppev/logger.h"

namespace cppev
{

namespace reactor
{

data_storage::data_storage(void *external_data_ptr)
    : on_accept(idle_handler),
      on_connect(idle_handler),
      on_read_complete(idle_handler),
      on_write_complete(idle_handler),
      on_closed(idle_handler),
      external_data_ptr(external_data_ptr)
{
}

data_storage::~data_storage() = default;

event_loop *data_storage::random_get_evlp()
{
    static std::random_device rd;
    static std::default_random_engine rde(rd());
    std::uniform_int_distribution<int> dist(0, evls.size() - 1);
    return evls[dist(rde)];
}

event_loop *data_storage::minloads_get_evlp()
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

void *data_storage::external_data() noexcept
{
    return external_data_ptr;
}

const void *data_storage::external_data() const noexcept
{
    return external_data_ptr;
}

void async_write(const std::shared_ptr<socktcp> &iopt)
{
    data_storage *dp = reinterpret_cast<data_storage *>(iopt->evlp().data());
    if (!exception_guard([&iops = iopt] { iops->write_all(); }))
    {
        LOG_ERROR_FMT("Syscall write error for fd %d", iopt->fd());
    }
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
    return (reinterpret_cast<data_storage *>(iopt->evlp().data()))
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

const tcp_event_handler data_storage::idle_handler =
    [](const std::shared_ptr<socktcp> &) -> void {};

static void with_exception_handling(const std::string &thr_name,
                                    std::function<void()> handler)
{
    LOG_INFO_FMT("Thread %s starting", thr_name.c_str());
    try
    {
        handler();
    }
    catch (std::exception &e)
    {
        LOG_ERROR << e.what();
    }
    LOG_INFO_FMT("Thread %s ending", thr_name.c_str());
}

iohandler::iohandler(data_storage *data)
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
    data_storage *dp = reinterpret_cast<data_storage *>(iop->evlp().data());
    if (!exception_guard([&iops = iopt] { iops->read_all(); }))
    {
        LOG_ERROR_FMT("Syscall read error for fd %d", iopt->fd());
    }
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
    data_storage *dp = reinterpret_cast<data_storage *>(iop->evlp().data());
    if (!exception_guard([&iops = iopt] { iops->write_all(); }))
    {
        LOG_ERROR_FMT("Syscall write error for fd %d", iopt->fd());
    }
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

void iohandler::on_conn_establish(const std::shared_ptr<io> &iop,
                                  init_checker checker,
                                  tcp_event_handler handler)
{
    std::shared_ptr<socktcp> iopt = std::dynamic_pointer_cast<socktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }
    iopt->evlp().fd_remove_and_deactivate(iop, fd_event::fd_writable);

    if (!checker(iop))
    {
        return;
    }

    // The sequence CANNOT be changed, since on_accept may call async_write
    iopt->evlp().fd_register(iop, fd_event::fd_writable,
                             iohandler::on_writable);
    handler(iopt);
    iopt->evlp().fd_register_and_activate(iop, fd_event::fd_readable,
                                          iohandler::on_readable);
    LOG_INFO_FMT("Connected socket %d initialized", iop->fd());
}

event_loop &iohandler::evlp()
{
    return evlp_;
}

void iohandler::run_without_exception_handling()
{
    evlp_.loop_forever();
}

void iohandler::run_impl()
{
    with_exception_handling(
        "iohandler",
        std::bind(&iohandler::run_without_exception_handling, this));
}

void iohandler::shutdown()
{
    if (!evlp_.stop_loop(sysconfig::reactor_shutdown_timeout))
    {
        LOG_WARNING_FMT("iohandler shutdown wait timeout");
    }
}

acceptor::acceptor(data_storage *data)
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
    LOG_INFO_FMT("Listening socket %d working in %s %d", sock->fd(),
                 ip ? ip : "localhost", port);
}

void acceptor::listen_unix(const std::string &path, bool remove)
{
    std::shared_ptr<socktcp> sock = io_factory::get_socktcp(family::local);
    sock->bind_unix(path, remove);
    sock->listen();
    socks_.push_back(sock);
    LOG_INFO_FMT("Listening socket %d working in %s", sock->fd(), path.c_str());
}

void acceptor::on_acpt_readable(const std::shared_ptr<io> &iop)
{
    std::shared_ptr<socktcp> iopt = std::dynamic_pointer_cast<socktcp>(iop);
    if (iopt == nullptr)
    {
        throw_logic_error("dynamic_pointer_cast error");
    }
    data_storage *dp = reinterpret_cast<data_storage *>(iopt->evlp().data());

    static iohandler::init_checker checker = [](const std::shared_ptr<io> &)
    { return true; };

    std::vector<std::shared_ptr<socktcp>> conns = iopt->accept();

    for (auto &conn : conns)
    {
        LOG_INFO_FMT("Listening socket %d accepted new socket %d", iopt->fd(),
                     conn->fd());
        dp->minloads_get_evlp()->fd_register_and_activate(
            std::static_pointer_cast<io>(conn), fd_event::fd_writable,
            std::bind(iohandler::on_conn_establish, std::placeholders::_1,
                      checker, dp->on_accept));
    }
}

void acceptor::run_without_exception_handling()
{
    for (auto &sock : socks_)
    {
        evlp_.fd_register_and_activate(std::static_pointer_cast<io>(sock),
                                       fd_event::fd_readable,
                                       acceptor::on_acpt_readable);
    }
    evlp_.loop_forever();
}

void acceptor::run_impl()
{
    with_exception_handling(
        "acceptor", std::bind(&acceptor::run_without_exception_handling, this));
}

void acceptor::shutdown()
{
    if (!evlp_.stop_loop(sysconfig::reactor_shutdown_timeout))
    {
        LOG_WARNING_FMT("acceptor shutdown wait timeout");
    }
}

connector::connector(data_storage *data)
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
        hosts_[h] += t;
    }

    wrp_->wbuffer().put_string("0");
    if (!exception_guard([&iops = this->wrp_] { iops->write_all(1); }))
    {
        LOG_ERROR_FMT("Syscall write error for fd %d", wrp_->fd());
    }
}

void connector::on_pipe_readable(const std::shared_ptr<io> &iop)
{
    stream *iops = dynamic_cast<stream *>(iop.get());
    if (iops == nullptr)
    {
        throw_logic_error("dynamic_cast error");
    }
    data_storage *dp = reinterpret_cast<data_storage *>(iops->evlp().data());

    connector *pseudo_this =
        reinterpret_cast<connector *>(iops->evlp().owner());

    iohandler::init_checker checker =
        [pseudo_this](const std::shared_ptr<io> &iop) -> bool
    {
        std::shared_ptr<socktcp> iopt = std::dynamic_pointer_cast<socktcp>(iop);
        bool ret = iopt->check_connect();
        if (!ret)
        {
            std::tuple<std::string, int, family> h = iopt->target_uri();

            {
                std::unique_lock<std::mutex> _(pseudo_this->lock_);
                pseudo_this->failures_[h] += 1;
            }
            iopt->evlp().fd_clean(iop);
            iopt->close();
            if (family::local == std::get<2>(h))
            {
                LOG_WARNING_FMT("Connect %s failed when checking writable",
                                std::get<0>(h).c_str());
            }
            else
            {
                LOG_WARNING_FMT("Connect %s %d failed when checking writable",
                                std::get<0>(h).c_str(), std::get<1>(h));
            }
        }
        return ret;
    };

    if (!exception_guard([&iops] { iops->read_all(1); }))
    {
        LOG_ERROR_FMT("Syscall read error for fd %d", iops->fd());
    }

    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash>
        hosts;
    {
        std::unique_lock<std::mutex> _(pseudo_this->lock_);
        pseudo_this->hosts_.swap(hosts);
    }

    for (auto iter = hosts.begin(); iter != hosts.end(); ++iter)
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
                    std::bind(iohandler::on_conn_establish,
                              std::placeholders::_1, checker, dp->on_connect));
            }
            else
            {
                {
                    std::unique_lock<std::mutex> _(pseudo_this->lock_);
                    pseudo_this->failures_[iter->first] += 1;
                }
                std::error_code err_code(errno, std::system_category());
                if (std::get<2>(iter->first) == family::local)
                {
                    LOG_WARNING_FMT(
                        "Connect %s failed with syscall errno %d : %s",
                        std::get<0>(iter->first).c_str(), err_code.value(),
                        err_code.message().c_str());
                }
                else
                {
                    LOG_WARNING_FMT(
                        "Connect %s %d failed with syscall errno %d : %s",
                        std::get<0>(iter->first).c_str(),
                        std::get<1>(iter->first), err_code.value(),
                        err_code.message().c_str());
                }
            }
        }
    }
}

void connector::run_without_exception_handling()
{
    evlp_.fd_register_and_activate(std::static_pointer_cast<io>(rdp_),
                                   fd_event::fd_readable,
                                   connector::on_pipe_readable);
    evlp_.loop_forever();
}

void connector::run_impl()
{
    with_exception_handling(
        "connector",
        std::bind(&connector::run_without_exception_handling, this));
}

void connector::shutdown()
{
    if (!evlp_.stop_loop(sysconfig::reactor_shutdown_timeout))
    {
        LOG_WARNING_FMT("connector shutdown wait timeout");
    }
}

tcp_common::tcp_common(int iohandler_num, void *external_data)
    : data_(external_data), tp_(iohandler_num, &data_)
{
    for (auto &thr : tp_)
    {
        data_.evls.push_back(&(thr.evlp()));
    }
}

tcp_common::~tcp_common() = default;

void tcp_common::set_on_read_complete(const tcp_event_handler &handler)
{
    data_.on_read_complete = handler;
}

void tcp_common::set_on_write_complete(const tcp_event_handler &handler)
{
    data_.on_write_complete = handler;
}

void tcp_common::set_on_closed(const tcp_event_handler &handler)
{
    data_.on_closed = handler;
}

tcp_server::tcp_server(int iohandler_num, bool single_acceptor,
                       void *external_data)
    : tcp_common(iohandler_num, external_data),
      single_acceptor_(single_acceptor)
{
}

tcp_server::~tcp_server() = default;

void tcp_server::set_on_accept(const tcp_event_handler &handler)
{
    data_.on_accept = handler;
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
    tcp_common::run(acpts_);
}

void tcp_server::shutdown()
{
    tcp_common::shutdown(acpts_);
}

tcp_client::tcp_client(int iohandler_num, int connector_num,
                       void *external_data)
    : tcp_common(iohandler_num, external_data)
{
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

void tcp_client::add(const std::string &ip, int port, family f, int t)
{
    int div = t / conts_.size();
    int mod = t % conts_.size();
    for (auto &cont : conts_)
    {
        cont->add(ip, port, f, div);
    }
    if (mod)
    {
        static std::random_device rd;
        static std::default_random_engine rde(rd());
        std::uniform_int_distribution<int> dist(0, conts_.size() - 1);
        conts_[dist(rde)]->add(ip, port, f, mod);
    }
}

void tcp_client::add_unix(const std::string &path, int t)
{
    add(path, 0, family::local, t);
}

void tcp_client::run()
{
    tcp_common::run(conts_);
}

void tcp_client::shutdown()
{
    tcp_common::shutdown(conts_);
}

}  // namespace reactor

}  // namespace cppev
