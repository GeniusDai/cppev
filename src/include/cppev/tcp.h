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

    tp_shared_data(const tp_shared_data &) = delete;
    tp_shared_data &operator=(const tp_shared_data &) = delete;
    tp_shared_data(tp_shared_data &&) = delete;
    tp_shared_data &operator=(tp_shared_data &&) = delete;

    ~tp_shared_data() = default;

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

    tcp_server *server_ptr() const
    {
        return reinterpret_cast<tcp_server *>(ptr);
    }

    tcp_client *client_ptr() const
    {
        return reinterpret_cast<tcp_client *>(ptr);
    }

private:
    friend class tcp_server;
    friend class tcp_client;
    friend class connector;

    // Event loops of thread pool, used for task assign
    std::vector<event_loop *> evls;

    void *ptr;
};


class iohandler
: public runnable
{
    friend class tcp_server;
    friend class tcp_client;
public:
    explicit iohandler(tp_shared_data *data)
    : evp_(std::shared_ptr<event_loop>(new event_loop(
        reinterpret_cast<void *>(data), reinterpret_cast<void *>(this))))
    {}

    iohandler(const iohandler &) = delete;
    iohandler &operator=(const iohandler &) = delete;
    iohandler(iohandler &&) =delete;
    iohandler &operator=(iohandler &&) = delete;

    virtual ~iohandler() = default;

    static void on_readable(std::shared_ptr<nio> iop);

    static void on_writable(std::shared_ptr<nio> iop);

    // Connect socket is writable, this cb will be executed immediately
    // by one thread of the pool to do init jobs
    static void on_acpt_writable(std::shared_ptr<nio> iop);

    // Connect socket is writable, indicating connection is ready for check, this
    // cb will be executed by one thread of the pool to do init jobs
    static void on_cont_writable(std::shared_ptr<nio> iop);

    void run_impl() override
    {
        evp_->loop();
    }

private:
    std::shared_ptr<event_loop> evp_;

    // hosts that failed to connect
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> failures_;
};

class acceptor
: public runnable
{
public:
    explicit acceptor(tp_shared_data *data)
    : evp_(std::shared_ptr<event_loop>(new event_loop(
        reinterpret_cast<void *>(data), reinterpret_cast<void *>(this))))
    {}

    acceptor(const acceptor &) = delete;
    acceptor &operator=(const acceptor &) = delete;
    acceptor(acceptor &&) = delete;
    acceptor &operator=(acceptor &&) = delete;

    virtual ~acceptor() = default;

    // Specify listening socket's port and family
    void listen(int port, family f, const char *ip = nullptr);

    // Specify unix domain listening socket's path
    void listen_unix(const std::string &path);

    // Listening socket is readable, indicating new client arrives, this cb will be executed
    // by accept thread to accept connection and assign connection to thread pool
    static void on_acpt_readable(std::shared_ptr<nio> iop);

    // Register readable to event loop and start loop
    void run_impl() override;

private:
    // Event loop
    std::shared_ptr<event_loop> evp_;

    // Listening socket
    std::shared_ptr<nsocktcp> sock_;
};

class tcp_server final
{
public:
    explicit tcp_server(int thr_num);

    tcp_server(const tcp_server &) = delete;
    tcp_server &operator=(const tcp_server &) = delete;
    tcp_server(tcp_server &&) = delete;
    tcp_server &operator=(tcp_server &&) = delete;

    ~tcp_server() = default;

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
    explicit connector(tp_shared_data *data)
    : evp_(std::shared_ptr<event_loop>(new event_loop(
        reinterpret_cast<void *>(data), reinterpret_cast<void *>(this))))
    {
        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            throw_system_error("pipe error");
        }
        rdp_ = std::shared_ptr<nstream>(new nstream(pipefd[0]));
        wrp_ = std::shared_ptr<nstream>(new nstream(pipefd[1]));
    }

    connector(const connector &) = delete;
    connector &operator=(const connector &) = delete;
    connector(connector &&) = delete;
    connector &operator=(connector &&) = delete;

    virtual ~connector() = default;

    // Add connection task (ip, port, family)
    void add(const std::string &ip, int port, family f, int t = 1);

    // Add connection task (path, 0, family::local)
    void add_unix(const std::string &path, int t = 1);

    // Pipe fd is readable, indicating new task added, this cb will be executed by
    // connect thread to init connection
    static void on_pipe_readable(std::shared_ptr<nio> iop);

    // Register readable to event loop and start loop
    void run_impl() override;

private:
    // Event loop
    std::shared_ptr<event_loop> evp_;

    // Pipe write end
    std::shared_ptr<nstream> wrp_;

    // Pipe read end
    std::shared_ptr<nstream> rdp_;

    // hosts waiting for connecting
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> hosts_;

    // hosts that failed to connect
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> failures_;
};

class tcp_client final
{
public:
    explicit tcp_client(int thr_num, int cont_num = 1);

    tcp_client(const tcp_client &) = delete;
    tcp_client &operator=(const tcp_client &) = delete;
    tcp_client(tcp_client &&) = delete;
    tcp_client &operator=(tcp_client &&) = delete;

    ~tcp_client() = default;

    void add(const std::string &ip, int port, family f, int t = 1);

    void add_unix(const std::string &path, int t = 1);

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

    std::vector<std::shared_ptr<connector> > conts_;

    std::shared_ptr<thread_pool<iohandler, tp_shared_data *> > tp_;
};

}   // namespace tcp

using namespace tcp;

}   // namespace cppev

#endif  // tcp.h
