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
#include "cppev/nio.h"
#include "cppev/event_loop.h"
#include "cppev/runnable.h"
#include "cppev/thread_pool.h"
#include "cppev/async_logger.h"

namespace cppev
{

namespace reactor
{

// Callback function type
using tcp_event_handler = std::function<void(const std::shared_ptr<nsocktcp> &)>;

// Async write data in write buffer
void async_write(const std::shared_ptr<nsocktcp> &iopt);

// Safely close tcp socket
void safely_close(const std::shared_ptr<nsocktcp> &iopt);

// Get external data of reactor server and client
void *external_data(const std::shared_ptr<nsocktcp> &iopt);

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
private:
    friend class tcp_server;
    friend class tcp_client;

    // Idle function for callback
    static const tcp_event_handler idle_handler;

public:
    // All the five callbacks will be executed by worker thread
    tp_shared_data(void *external_data_ptr = nullptr)
    :
        on_accept(idle_handler),
        on_connect(idle_handler),
        on_read_complete(idle_handler),
        on_write_complete(idle_handler),
        on_closed(idle_handler),
        external_data_ptr(external_data_ptr)
    {
    }

    tp_shared_data(const tp_shared_data &) = delete;
    tp_shared_data &operator=(const tp_shared_data &) = delete;
    tp_shared_data(tp_shared_data &&) = delete;
    tp_shared_data &operator=(tp_shared_data &&) = delete;

    ~tp_shared_data() = default;

    // When tcp server accepts new connection
    tcp_event_handler on_accept;

    // When tcp client establishes new connection
    tcp_event_handler on_connect;

    // When read from tcp connection completes
    tcp_event_handler on_read_complete;

    // When write to tcp connection completes
    tcp_event_handler on_write_complete;

    // When tcp socket closed by opposite host
    tcp_event_handler on_closed;

    // Load balance algorithm : choose worker randomly
    event_loop *random_get_evlp();

    // Load balance algorithm : choose worker which has minimum loads
    event_loop *minloads_get_evlp();

    // External data defined by user
    void *external_data()
    {
        return external_data_ptr;
    }

private:
    // Event loops of thread pool, used for task assign
    std::vector<event_loop *> evls;

    // Pointer to reactor server or client
    void *reactor_ptr;

    // Pointer to external data may be used by handler registered by user
    void *external_data_ptr;
};


class iohandler
: public runnable
{
    friend class tcp_server;
    friend class tcp_client;
public:
    explicit iohandler(tp_shared_data *data)
    : evlp_(std::make_shared<event_loop>(reinterpret_cast<void *>(data), reinterpret_cast<void *>(this)))
    {
    }

    iohandler(const iohandler &) = delete;
    iohandler &operator=(const iohandler &) = delete;
    iohandler(iohandler &&) =delete;
    iohandler &operator=(iohandler &&) = delete;

    virtual ~iohandler() = default;

    // Connected socket that has been registered to thread pool is readable
    static void on_readable(const std::shared_ptr<nio> &iop);

    // Connected socket that has been registered to thread pool is writable
    static void on_writable(const std::shared_ptr<nio> &iop);

    // Connected socket is writable, this callback is registered by listening thread and
    // will be executed by one thread of the pool to do init jobs
    static void on_acpt_writable(const std::shared_ptr<nio> &iop);

    // Connected socket is writable, this callback is registered by connecting thread and
    // will be executed by one thread of the pool to check the connection and do init jobs
    static void on_cont_writable(const std::shared_ptr<nio> &iop);

    void run_impl() override
    {
        evlp_->loop();
    }

private:
    // Event loop
    std::shared_ptr<event_loop> evlp_;

    // Hosts failed in the SO_ERROR check
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> failures_;
};


class acceptor
: public runnable
{
public:
    explicit acceptor(tp_shared_data *data)
    : evlp_(std::make_shared<event_loop>(reinterpret_cast<void *>(data), reinterpret_cast<void *>(this)))
    {
    }

    acceptor(const acceptor &) = delete;
    acceptor &operator=(const acceptor &) = delete;
    acceptor(acceptor &&) = delete;
    acceptor &operator=(acceptor &&) = delete;

    virtual ~acceptor() = default;

    // Specify listening socket's port and family
    void listen(int port, family f, const char *ip = nullptr);

    // Specify unix domain listening socket's path
    void listen_unix(const std::string &path, bool remove = false);

    // Listening socket is readable, indicating new client arrives, this callback will be executed
    // by accept thread to accept connection and assign connection to thread pool
    static void on_acpt_readable(const std::shared_ptr<nio> &iop);

    // Register readable to event loop and start loop
    void run_impl() override;

private:
    // Event loop
    std::shared_ptr<event_loop> evlp_;

    // Listening socket
    std::shared_ptr<nsocktcp> sock_;
};


class tcp_server final
{
public:
    explicit tcp_server(int thr_num, void *external_data = nullptr);

    tcp_server(const tcp_server &) = delete;
    tcp_server &operator=(const tcp_server &) = delete;
    tcp_server(tcp_server &&) = delete;
    tcp_server &operator=(tcp_server &&) = delete;

    ~tcp_server() = default;

    void listen(int port, family f, const char *ip = nullptr)
    {
        acpts_.push_back(std::make_shared<acceptor>(data_.get()));
        acpts_.back()->listen(port, f, ip);
    }

    void listen_unix(const std::string &path, bool remove = false)
    {
        acpts_.push_back(std::make_shared<acceptor>(data_.get()));
        acpts_.back()->listen_unix(path, remove);
    }

    void set_on_accept(const tcp_event_handler &handler)
    {
        data_->on_accept = handler;
    }

    void set_on_read_complete(const tcp_event_handler &handler)
    {
        data_->on_read_complete = handler;
    }

    void set_on_write_complete(const tcp_event_handler &handler)
    {
        data_->on_write_complete = handler;
    }

    void set_on_closed(const tcp_event_handler &handler)
    {
        data_->on_closed = handler;
    }

    void run();

private:
    // Thread pool shared data
    std::shared_ptr<tp_shared_data> data_;

    // Listening threads
    std::vector<std::shared_ptr<acceptor> > acpts_;

    // Worker threads
    std::shared_ptr<thread_pool<iohandler, tp_shared_data *> > tp_;
};


class connector
: public runnable
{
public:
    explicit connector(tp_shared_data *data)
    : evlp_(std::make_shared<event_loop>(reinterpret_cast<void *>(data), reinterpret_cast<void *>(this)))
    {
        auto pipes = nio_factory::get_pipes();
        rdp_ = pipes[0];
        wrp_ = pipes[1];
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

    // Pipe fd is readable, indicating new task added, this callback will be executed by
    // connect thread to execute the connection task and assign connection to thread pool
    static void on_pipe_readable(const std::shared_ptr<nio> &iop);

    // Register readable to event loop and start loop
    void run_impl() override;

private:
    // Event loop
    std::shared_ptr<event_loop> evlp_;

    // Pipe write end
    std::shared_ptr<nstream> wrp_;

    // Pipe read end
    std::shared_ptr<nstream> rdp_;

    // Hosts waiting for connecting
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> hosts_;

    // Hosts failed in the connect syscall
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> failures_;
};


class tcp_client final
{
public:
    explicit tcp_client(int thr_num, int cont_num = 1, void *external_data = nullptr);

    tcp_client(const tcp_client &) = delete;
    tcp_client &operator=(const tcp_client &) = delete;
    tcp_client(tcp_client &&) = delete;
    tcp_client &operator=(tcp_client &&) = delete;

    ~tcp_client() = default;

    void add(const std::string &ip, int port, family f, int t = 1);

    void add_unix(const std::string &path, int t = 1);

    void set_on_connect(const tcp_event_handler &handler)
    {
        data_->on_connect = handler;
    }

    void set_on_read_complete(const tcp_event_handler &handler)
    {
        data_->on_read_complete = handler;
    }

    void set_on_write_complete(const tcp_event_handler &handler)
    {
        data_->on_write_complete = handler;
    }

    void set_on_closed(const tcp_event_handler &handler)
    {
        data_->on_closed = handler;
    }

    void run();

private:
    // Thread pool shared data
    std::shared_ptr<tp_shared_data> data_;

    // Connecting threads
    std::vector<std::shared_ptr<connector> > conts_;

    // Worker threads
    std::shared_ptr<thread_pool<iohandler, tp_shared_data *> > tp_;
};

}   // namespace reactor

}   // namespace cppev

#endif  // tcp.h
