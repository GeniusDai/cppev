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

struct host_hash
{
    size_t operator()(const std::tuple<std::string, int, family> &h) const
    {
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

    event_loop *random_get_evlp();

    event_loop *minloads_get_evlp();

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

class tcp_server final {
public:
    tcp_server(int thr_num);

    void listen(const int port, family f, const char *ip = nullptr)
    { acpt_->listen(port, f, ip); }

    void set_on_accept(fd_handler handler)
    { data_->on_accept = handler; }

    void set_on_read_complete(fd_handler handler)
    { data_->on_read_complete = handler; }

    void set_on_write_complete(fd_handler handler)
    { data_->on_write_complete = handler; }

    void set_on_closed(fd_handler handler)
    { data_->on_closed = handler; }

    void run();

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

    connector(std::shared_ptr<event_loop> evp) : evp_(evp)
    {
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

class tcp_client final {
public:
    tcp_client(int thr_num);

    void add(const std::string ip, const int port, family f, int t = 1)
    { cont_->add(ip, port, f, t); }

    void set_on_connect(fd_handler handler)
    { data_->on_connect = handler; }

    void set_on_read_complete(fd_handler handler)
    { data_->on_read_complete = handler; }

    void set_on_write_complete(fd_handler handler)
    { data_->on_write_complete = handler; }

    void set_on_closed(fd_handler handler)
    { data_->on_closed = handler; }

    void run();

private:
    std::shared_ptr<tp_shared_data> data_;

    std::shared_ptr<connector> cont_;

    std::shared_ptr<thread_pool<iohandler, tp_shared_data *> > tp_;
};

constexpr auto async_write = iohandler::async_write;

}

#endif
