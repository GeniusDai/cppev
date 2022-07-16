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

namespace cppev
{

namespace tcp
{

// Callback function type
typedef void(*tcp_event_cb)(std::shared_ptr<nsocktcp>);

// Async write data in write buffer
void async_write(std::shared_ptr<nsocktcp> iopt);

// Safely close tcp socket
void safely_close(std::shared_ptr<nsocktcp> iopt);

class acceptor;
class connector;
class iohandler;
class tcp_server;
class tcp_client;

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
struct tp_shared_data final
{
public:
    tp_shared_data();

    ~tp_shared_data()
    {}

    // Used by acceptor when new connection arrived
    tcp_event_cb on_accept;

    // Used by connector when connection established
    tcp_event_cb on_connect;

    // Used by iohandler when read complete
    tcp_event_cb on_read_complete;

    // Used by iohandler when write complete
    tcp_event_cb on_write_complete;

    // Used by iohandler when socket closed
    tcp_event_cb on_closed;

    // Load balance algorithm : choose worker randomly
    event_loop *random_get_evlp();

    // Load balance algorithm : choose worker which has minimum loads
    event_loop *minloads_get_evlp();

private:
    friend class tcp_server;
    friend class tcp_client;
    friend class connector;

    // Event loops of thread pool, used for task assign
    std::vector<event_loop *> evls;

    // Used only by connector : hosts waiting for connecting
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> hosts;

    // Used only by connector : hosts that failed to connect
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> failures;
};


class iohandler
: public runnable
{
    friend class tcp_server;
    friend class tcp_client;
public:
    iohandler(tp_shared_data *data)
    : iohandler(std::shared_ptr<event_loop>(new event_loop(static_cast<void *>(data))))
    {}

    iohandler(std::shared_ptr<event_loop> evp)
    : evp_(evp)
    {}

    virtual ~iohandler()
    {}

    static void on_readable(std::shared_ptr<nio> iop);

    static void on_writable(std::shared_ptr<nio> iop);

    void run_impl() override
    {
        evp_->loop();
    }

private:
    std::shared_ptr<event_loop> evp_;
};

class acceptor
: public runnable
{
public:
    acceptor(tp_shared_data *data)
    : acceptor(std::shared_ptr<event_loop>(new event_loop(static_cast<void *>(data))))
    {}

    acceptor(std::shared_ptr<event_loop> evp)
    : evp_(evp)
    {}

    virtual ~acceptor()
    {}

    // Specify listening socket's port and family
    void listen(int port, family f, const char *ip = nullptr);

    // Specify unix domain listening socket's path
    void listen_unix(const std::string &path);

    // Listening socket is readable, indicating new client arrives, this cb will be executed
    // by accept thread to accept connection and assign connection to thread pool
    static void on_acpt_readable(std::shared_ptr<nio> iop);

    // Connect socket is writable, this cb will be executed immediately
    // by one thread of the pool to do init jobs
    static void on_conn_writable(std::shared_ptr<nio> iop);

    // Register readable to event loop and start loop
    void run_impl() override;

private:
    // Event loop
    std::shared_ptr<event_loop> evp_;

    // Listening socket
    std::vector<std::shared_ptr<nsocktcp> > socks_;
};

class tcp_server final
: public uncopyable
{
public:
    tcp_server(int thr_num);

    void listen(int port, family f, const char *ip = nullptr)
    {
        acpts_.push_back(std::shared_ptr<acceptor>(new acceptor(data_.get())));
        acpts_.back()->listen(port, f, ip);
    }

    void listen_unix(const std::string &path)
    {
        acpts_.push_back(std::shared_ptr<acceptor>(new acceptor(data_.get())));
        acpts_.back()->listen_unix(path);
    }

    void set_on_accept(tcp_event_cb handler)
    {
        data_->on_accept = handler;
    }

    void set_on_read_complete(tcp_event_cb handler)
    {
        data_->on_read_complete = handler;
    }

    void set_on_write_complete(tcp_event_cb handler)
    {
        data_->on_write_complete = handler;
    }

    void set_on_closed(tcp_event_cb handler)
    {
        data_->on_closed = handler;
    }

    void run();

private:
    std::shared_ptr<tp_shared_data> data_;

    std::vector<std::shared_ptr<acceptor> > acpts_;

    std::shared_ptr<thread_pool<iohandler, tp_shared_data *> > tp_;
};


class connector
: public runnable
{
public:
    connector(tp_shared_data *data)
    : connector(std::shared_ptr<event_loop>(new event_loop(static_cast<void *>(data))))
    {}

    connector(std::shared_ptr<event_loop> evp)
    : evp_(evp)
    {
        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            throw_system_error("pipe error");
        }
        rdp_ = std::shared_ptr<nstream>(new nstream(pipefd[0]));
        wrp_ = std::shared_ptr<nstream>(new nstream(pipefd[1]));
    }

    virtual ~connector()
    {}

    // Add connection task (ip, port, family)
    void add(const std::string &ip, int port, family f, int t = 1);

    // Add connection task (path, 0, family::local)
    void add_unix(const std::string &path, int t = 1);

    // Pipe fd is readable, indicating new task added, this cb will be executed by
    // connect thread to init connection
    static void on_pipe_readable(std::shared_ptr<nio> iop);

    // Connect socket is writable, indicating connection is ready for check, this
    // cb will be executed by one thread of the pool to do init jobs
    static void on_conn_writable(std::shared_ptr<nio> iop);

    // Register readable to event loop and start loop
    void run_impl() override;

private:
    // Event loop
    std::shared_ptr<event_loop> evp_;

    // Pipe write end
    std::shared_ptr<nstream> wrp_;

    // Pipe read end
    std::shared_ptr<nstream> rdp_;
};

class tcp_client final
: public uncopyable
{
public:
    tcp_client(int thr_num);

    void add(const std::string &ip, int port, family f, int t = 1)
    {
        cont_->add(ip, port, f, t);
    }

    void add_unix(const std::string &path, int t = 1)
    {
        cont_->add_unix(path, t);
    }

    void set_on_connect(tcp_event_cb handler)
    {
        data_->on_connect = handler;
    }

    void set_on_read_complete(tcp_event_cb handler)
    {
        data_->on_read_complete = handler;
    }

    void set_on_write_complete(tcp_event_cb handler)
    {
        data_->on_write_complete = handler;
    }

    void set_on_closed(tcp_event_cb handler)
    {
        data_->on_closed = handler;
    }

    void run();

private:
    std::shared_ptr<tp_shared_data> data_;

    std::shared_ptr<connector> cont_;

    std::shared_ptr<thread_pool<iohandler, tp_shared_data *> > tp_;
};

}   // namespace tcp

using namespace tcp;

}   // namespace cppev

#endif  // tcp.h
