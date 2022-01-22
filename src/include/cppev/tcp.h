#ifndef _tcp_h_6C0224787A17_
#define _tcp_h_6C0224787A17_

#include <memory>
#include <queue>
#include <vector>
#include <random>
#include <type_traits>
#include <iostream>
#include <functional>
#include <signal.h>
#include "cppev/event_loop.h"
#include "cppev/runnable.h"
#include "cppev/thread_pool.h"
#include "cppev/async_logger.h"

namespace cppev {

class acceptor;
class connector;
class iohandler;
class tcp_server;
class tcp_client;

// Idle function for callback
auto idle_handler = [] (std::shared_ptr<nio> iop, event_loop *evp) -> void {};

using fd_handler = fd_event_cb;

struct host_hash {
    size_t operator()(const std::tuple<std::string, int, family> &h) const {
        size_t ret = 0;
        ret += std::hash<std::string>()(std::get<0>(h));
        ret += static_cast<size_t>(std::get<1>(h)) * 100;
        ret += static_cast<size_t>(std::get<2>(h)) * 10;
        return ret;
    }
};

// Data used for event loop initialization
struct tp_shared_data {
public:
    tp_shared_data() :
        on_accept(idle_handler),
        on_connect(idle_handler),
        on_read_complete(idle_handler),
        on_write_complete(idle_handler),
        on_closed(idle_handler)
    {}

    virtual ~tp_shared_data() {}

    // Used by acceptor when new connection arrived
    fd_handler on_accept;

    // Used by connector when connection established
    fd_handler on_connect;

    // Used by iohandler when read complete
    fd_handler on_read_complete;

    // Used by iohandler when write complete
    fd_handler on_write_complete;

    // Used by iohandler when socket closed
    fd_handler on_closed;

    event_loop *random_get_evlp() {
        std::random_device rd;
        std::default_random_engine rde(rd());
        std::uniform_int_distribution<int> dist(0, evls.size()-1);
        return evls[dist(rde)];
    }

private:
    friend class tcp_server;
    friend class tcp_client;
    friend class connector;

    // Event loops of thread pool, used for task assign
    std::vector<event_loop *> evls;

    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> hosts;

    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> failures;

    std::mutex lock;
};


class iohandler : public runnable {
    friend class tcp_server;
    friend class tcp_client;
public:
    iohandler(tp_shared_data *data)
    : iohandler(std::shared_ptr<event_loop>(new event_loop(static_cast<void *>(data))))
    {}

    iohandler(std::shared_ptr<event_loop> evp) : evp_(evp) {}

    virtual ~iohandler() {}

    static void async_write(std::shared_ptr<nio> iop, event_loop *evp);

    static void on_readable(std::shared_ptr<nio> iop, event_loop *evp);

    static void on_writable(std::shared_ptr<nio> iop, event_loop *evp);

    void run_impl() override { evp_->loop(); }

private:
    std::shared_ptr<event_loop> evp_;
};

void iohandler::async_write(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    if (iopt == nullptr) { throw_logic_error("dynamic_cast error"); }
    tp_shared_data *dp = static_cast<tp_shared_data *>(evp->data());
    iopt->write_all(sysconfig::buffer_io_step);
    if (0 == iopt->wbuf()->size()) {
        dp->on_write_complete(iop, evp);
    } else {
        evp->fd_remove(iop, false);
        evp->fd_register(iop, fd_event::fd_writable);
    }
}

void iohandler::on_readable(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    if (iopt == nullptr) { throw_logic_error("dynamic_cast error"); }
    tp_shared_data *dp = static_cast<tp_shared_data *>(evp->data());
    iopt->read_all(sysconfig::buffer_io_step);
    dp->on_read_complete(iop, evp);
    iopt->rbuf()->clear();
    if (iopt->eof() || iopt->is_reset()) {
        dp->on_closed(iop, evp);
        evp->fd_remove(iop, true);
    }
}

void iohandler::on_writable(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    if (iopt == nullptr) { throw_logic_error("dynamic_cast error"); }
    tp_shared_data *dp = static_cast<tp_shared_data *>(evp->data());
    iopt->write_all(sysconfig::buffer_io_step);
    if (0 == iopt->wbuf()->size()) {
        evp->fd_remove(iop, false);
        dp->on_write_complete(iop, evp);
        evp->fd_register(iop, fd_event::fd_readable);
    }
    if (iopt->eop() || iopt->is_reset()) {
        dp->on_closed(iop, evp);
        evp->fd_remove(iop, true);
    }
}


class acceptor : public runnable {
public:
    acceptor(tp_shared_data *data)
    : acceptor(std::shared_ptr<event_loop>(new event_loop(static_cast<void *>(data))))
    {}

    acceptor(std::shared_ptr<event_loop> evp) : evp_(evp) {}

    virtual ~acceptor() {}

    // Specify listening socket's port and family
    void listen(int port, family f, const char *ip = nullptr);

    // Listen socket readable, this function will be executed by acceptor-thread
    static void on_readable(std::shared_ptr<nio> iop, event_loop *evp);

    // This function will be executed by io-thread
    static void on_writable(std::shared_ptr<nio> iop, event_loop *evp);

    // Register readable to event loop and start loop
    void run_impl() override;

private:
    std::shared_ptr<nsocktcp> sock_;

    std::shared_ptr<event_loop> evp_;
};

void acceptor::listen(int port, family f, const char *ip) {
    sock_ = nio_factory::get_nsocktcp(f);
    sock_->listen(port, ip);
}

void acceptor::on_readable(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    if (iopt == nullptr) { throw_logic_error("dynamic_cast error"); }
    std::vector<std::shared_ptr<nsocktcp> > conns = iopt->accept();
    tp_shared_data *d = static_cast<tp_shared_data *>(evp->data());

    for (auto &p : conns) {
        log::info << "new fd " << p->fd() << " accepted" << log::endl;
        event_loop *io_evlp = d->random_get_evlp();
        io_evlp->fd_register(std::dynamic_pointer_cast<nio>(p),
            fd_event::fd_writable, acceptor::on_writable, true);
    }
}

void acceptor::on_writable(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    if (iopt == nullptr) { throw_logic_error("dynamic_cast error"); }
    tp_shared_data *d = static_cast<tp_shared_data *>(evp->data());
    evp->fd_remove(iop, true);
    // The sequence CANNOT be changed, since on_accept may call async_write
    evp->fd_register(iop, fd_event::fd_writable, iohandler::on_writable, false);
    d->on_accept(iop, evp);
    evp->fd_register(iop, fd_event::fd_readable, iohandler::on_readable, true);
}

void acceptor::run_impl() {
    evp_->fd_register(std::dynamic_pointer_cast<nio>(sock_),
        fd_event::fd_readable, acceptor::on_readable, true);
    evp_->loop();
}


class tcp_server final {
public:
    tcp_server(int thr_num) {
        data_ = std::shared_ptr<tp_shared_data>(new tp_shared_data);
        tp_ = std::shared_ptr<thread_pool<iohandler, tp_shared_data *> >
            (new thread_pool<iohandler, tp_shared_data *>(thr_num, data_.get()));
        for (int i = 0; i < tp_->size(); ++i) {
            data_->evls.push_back((*tp_)[i]->evp_.get());
        }
        acpt_ = std::shared_ptr<acceptor>(new acceptor(data_.get()));
    }

    void listen(const int port, family f, const char *ip = nullptr) {
        acpt_->listen(port, f, ip);
    }

    void run() {
        // thread pool must run first
        ignore_signal(SIGPIPE);
        tp_->run();
        acpt_->run();
        tp_->join();
        acpt_->join();
    }

    void set_on_accept(fd_handler handler)
    { data_->on_accept = handler; }

    void set_on_read_complete(fd_handler handler)
    { data_->on_read_complete = handler; }

    void set_on_write_complete(fd_handler handler)
    { data_->on_write_complete = handler; }

    void set_on_closed(fd_handler handler)
    { data_->on_closed = handler; }

private:
    std::shared_ptr<tp_shared_data> data_;

    std::shared_ptr<acceptor> acpt_;

    std::shared_ptr<thread_pool<iohandler, tp_shared_data *> > tp_;
};


class connector : public runnable {
public:
    connector(tp_shared_data *data)
    : connector(std::shared_ptr<event_loop>(new event_loop(static_cast<void *>(data))))
    {}

    connector(std::shared_ptr<event_loop> evp) : evp_(evp) {
        int pipefd[2];
        if (pipe(pipefd) == -1) { throw_system_error("pipe error"); }
        rdp_ = std::shared_ptr<nstream>(new nstream(pipefd[0]));
        wrp_ = std::shared_ptr<nstream>(new nstream(pipefd[1]));
    }

    virtual ~connector() {}

    // Add connection task (ip, port, family)
    void add(std::string ip, int port, family f, int t = 1);

    // New connect target added, this function will be executed by connector-thread
    static void on_readable(std::shared_ptr<nio> iop, event_loop *evp);

    // This function will be executed by io-thread
    static void on_writable(std::shared_ptr<nio> iop, event_loop *evp);

    // Register readable to event loop and start loop
    void run_impl() override;

private:
    std::shared_ptr<event_loop> evp_;

    std::shared_ptr<nstream> wrp_;

    std::shared_ptr<nstream> rdp_;
};

void connector::add(std::string ip, int port, family f, int t) {
    tp_shared_data *d = static_cast<tp_shared_data *>(evp_->data());
    std::unique_lock<std::mutex> lock(d->lock);
    auto h = std::make_tuple<>(ip, port, f);
    if (d->hosts.count(h)) { d->hosts[h] += t; }
    else { d->hosts[h] = t; }
    wrp_->wbuf()->put("0");
    wrp_->write_all(1);
}

void connector::on_readable(std::shared_ptr<nio> iop, event_loop *evp) {
    nstream *rdsp = dynamic_cast<nstream *>(iop.get());
    tp_shared_data *d = static_cast<tp_shared_data *>(evp->data());
    rdsp->read_all(1);
    std::unique_lock<std::mutex> lock(d->lock);
    for (auto iter = d->hosts.begin(); iter != d->hosts.end(); ) {
        for (int i = 0; i < iter->second; ++i) {
            std::shared_ptr<nsocktcp> sock = nio_factory::get_nsocktcp
                (std::get<2>(iter->first));
            sock->connect(std::get<0>(iter->first), std::get<1>(iter->first));
            event_loop *io_evlp = d->random_get_evlp();
            io_evlp->fd_register(std::dynamic_pointer_cast<nio>(sock),
                fd_event::fd_writable, connector::on_writable, true);
        }
        iter = d->hosts.erase(iter);
    }
}

void connector::on_writable(std::shared_ptr<nio> iop, event_loop *evp) {
    nsocktcp *iopt = dynamic_cast<nsocktcp *>(iop.get());
    tp_shared_data *d = static_cast<tp_shared_data *>(evp->data());
    evp->fd_remove(iop, true);      // remove previous callback
    if (!iopt->check_connect()) {
        std::tuple<std::string, int, family> h = iopt->connpeer();
        log::error << "connect failed with " << std::get<0>(h) << " "
            << std::get<1>(h) << log::endl;
        if (d->failures.count(h)) { d->failures[h] += 1; }
        else { d->failures[h] = 1; }
        return;
    }
    // The sequence CANNOT be changed since on_connect may call aysnc_write
    evp->fd_register(iop, fd_event::fd_writable, iohandler::on_writable, false);
    d->on_connect(iop, evp);
    evp->fd_register(iop, fd_event::fd_readable, iohandler::on_readable, true);
}

void connector::run_impl() {
    evp_->fd_register(std::dynamic_pointer_cast<nio>(rdp_),
        fd_event::fd_readable, connector::on_readable, true);
    evp_->loop();
}


class tcp_client final {
public:
    tcp_client(int thr_num) {
        data_ = std::shared_ptr<tp_shared_data>(new tp_shared_data);
        tp_ = std::shared_ptr<thread_pool<iohandler, tp_shared_data *> >
            (new thread_pool<iohandler, tp_shared_data *>(thr_num, data_.get()));
        for (int i = 0; i < tp_->size(); ++i) {
            data_->evls.push_back((*tp_)[i]->evp_.get());
        }
        cont_ = std::shared_ptr<connector>(new connector(data_.get()));
    }

    void add(const std::string ip, const int port, family f, int t = 1) {
        cont_->add(ip, port, f, t);
    }

    void run() {
        // thread pool must run first
        ignore_signal(SIGPIPE);
        tp_->run();
        cont_->run();
        tp_->join();
        cont_->join();
    }

    void set_on_connect(fd_handler handler)
    { data_->on_connect = handler; }

    void set_on_read_complete(fd_handler handler)
    { data_->on_read_complete = handler; }

    void set_on_write_complete(fd_handler handler)
    { data_->on_write_complete = handler; }

    void set_on_closed(fd_handler handler)
    { data_->on_closed = handler; }

private:
    std::shared_ptr<tp_shared_data> data_;

    std::shared_ptr<connector> cont_;

    std::shared_ptr<thread_pool<iohandler, tp_shared_data *> > tp_;
};

constexpr auto async_write = iohandler::async_write;

}

#endif
