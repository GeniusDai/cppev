#ifndef _cppev_tcp_h_6C0224787A17_
#define _cppev_tcp_h_6C0224787A17_

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

/*
    Q1: The architecture(Reactor-Impl)?
    A1: 1) Support tcp's client(web-crawler) and server.
        2) Using multi-threading(N+M), each thread works with an specific io-multiplexing.
        3) N threads deal with the syn_sent / listening socket, M threads(thread-pool) deal
           with the connected socket.
    Q2: The comparation of other implementation?
    A2: 1) Compared with "one or two io-multiplexing"(Reactor-Impl1).
            Reactor-Impl1 distributes sockets to the thread-pool when sockets are readable / writable,
            thread-pool registers sockets back after reading / writing. Assume for Linux tcp server
            Reactor-Impl1 lower the performance due to the epolls' RB-tree size expansion and being
            operated by multi-threads. Also why cannot the polling task becomes too heavy for only one
            thread? Although Reactor-Impl's coding is more complex.
        2) Compared with "nginx"(Reactor-Impl2).
            Reactor-Impl2 doesn't treat listening socket's event as higher priority, but Reactor-Impl does.
 */

namespace cppev
{

namespace reactor
{

// Callback function type.
using tcp_event_handler = std::function<void(const std::shared_ptr<nsocktcp> &)>;

// Async write data in write buffer.
void async_write(const std::shared_ptr<nsocktcp> &iopt);

// Safely close tcp socket.
void safely_close(const std::shared_ptr<nsocktcp> &iopt);

// Get external data of reactor server and client.
void *external_data(const std::shared_ptr<nsocktcp> &iopt);

class acceptor;
class connector;
class iohandler;
class tcp_server;
class tcp_client;

struct host_hash
{
    size_t operator()(const std::tuple<std::string, int, family> &h) const;
};

// Data used for event loop initialization.
struct tp_shared_data final
{
private:
    friend class tcp_server;
    friend class tcp_client;

    // Idle function for callback.
    static const tcp_event_handler idle_handler;

public:
    // All the five callbacks will be executed by worker thread.
    explicit tp_shared_data(void *external_data_ptr);

    tp_shared_data(const tp_shared_data &) = delete;
    tp_shared_data &operator=(const tp_shared_data &) = delete;
    tp_shared_data(tp_shared_data &&) = delete;
    tp_shared_data &operator=(tp_shared_data &&) = delete;

    ~tp_shared_data();

    // When tcp server accepts new connection.
    tcp_event_handler on_accept;

    // When tcp client establishes new connection.
    tcp_event_handler on_connect;

    // When read from tcp connection completes.
    tcp_event_handler on_read_complete;

    // When write to tcp connection completes.
    tcp_event_handler on_write_complete;

    // When tcp socket is closed by opposite host.
    tcp_event_handler on_closed;

    // Load balance algorithm : choose worker randomly.
    event_loop *random_get_evlp();

    // Load balance algorithm : choose worker which has minimum loads.
    event_loop *minloads_get_evlp();

    // External data defined by user.
    void *external_data() noexcept;

    const void *external_data() const noexcept;

private:
    // Event loops of thread pool, used for task assign.
    std::vector<event_loop *> evls;

    // Pointer to external data may be used by handler registered by user.
    void *external_data_ptr;
};


class iohandler final
: public runnable
{
    friend class tcp_server;
    friend class tcp_client;
public:
    explicit iohandler(tp_shared_data *data);

    iohandler(const iohandler &) = delete;
    iohandler &operator=(const iohandler &) = delete;
    iohandler(iohandler &&) =delete;
    iohandler &operator=(iohandler &&) = delete;

    ~iohandler();

    // Connected socket that has been registered to thread pool is readable.
    static void on_readable(const std::shared_ptr<nio> &iop);

    // Connected socket that has been registered to thread pool is writable.
    static void on_writable(const std::shared_ptr<nio> &iop);

    // Connected socket is writable, this callback is registered by listening thread and
    // will be executed by one thread of the pool to do init jobs.
    static void on_acpt_writable(const std::shared_ptr<nio> &iop);

    // Connected socket is writable, this callback is registered by connecting thread and
    // will be executed by one thread of the pool to check the connection and do init jobs.
    static void on_cont_writable(const std::shared_ptr<nio> &iop);

    // Run io handling.
    void run_impl() override;

    // Shutdown io eventloop.
    void shutdown();

private:
    // Event loop.
    event_loop evlp_;

    // Hosts failed in the SO_ERROR check.
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> failures_;
};


class acceptor final
: public runnable
{
public:
    explicit acceptor(tp_shared_data *data);

    acceptor(const acceptor &) = delete;
    acceptor &operator=(const acceptor &) = delete;
    acceptor(acceptor &&) = delete;
    acceptor &operator=(acceptor &&) = delete;

    ~acceptor();

    // Listening socket is readable, indicating new client arrives, this callback will be
    // executed by accept thread to accept connection and assign connection to thread pool.
    static void on_acpt_readable(const std::shared_ptr<nio> &iop);

    // Register readable to event loop and start loop.
    void run_impl() override;

    // Specify listening socket's port and family.
    void listen(int port, family f, const char *ip);

    // Specify unix domain listening socket's path.
    void listen_unix(const std::string &path, bool remove);

    // Shutdown io eventloop.
    void shutdown();

private:
    // Event loop.
    event_loop evlp_;

    // Listening socket.
    std::vector<std::shared_ptr<nsocktcp>> socks_;
};


class connector final
: public runnable
{
public:
    explicit connector(tp_shared_data *data);

    connector(const connector &) = delete;
    connector &operator=(const connector &) = delete;
    connector(connector &&) = delete;
    connector &operator=(connector &&) = delete;

    ~connector();

    // Pipe fd is readable, indicating new task added, this callback will be executed by
    // connect thread to execute the connection task and assign connection to thread pool.
    static void on_pipe_readable(const std::shared_ptr<nio> &iop);

    // Register readable to event loop and start loop.
    void run_impl() override;

    // Add connection task (ip, port, family).
    void add(const std::string &ip, int port, family f, int t);

    // Add connection task (path, 0, family::local).
    void add_unix(const std::string &path, int t);

    // Shutdown io eventloop.
    void shutdown();

private:
    // Event loop.
    event_loop evlp_;

    // Protects hosts_.
    std::mutex lock_;

    // Pipe write end.
    std::shared_ptr<nstream> wrp_;

    // Pipe read end.
    std::shared_ptr<nstream> rdp_;

    // Hosts waiting for connecting.
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> hosts_;

    // Hosts failed in the connect syscall.
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash> failures_;
};


class tcp_server final
{
public:
    // Construct tcp server
    // @param iohandler_num      IO thread pool size.
    // @param single_acceptor    Whether using one acceptor for all listening socket.
    // @param external_data      External data pointer.
    explicit tcp_server(int iohandler_num, bool single_acceptor=true, void *external_data=nullptr);

    tcp_server(const tcp_server &) = delete;
    tcp_server &operator=(const tcp_server &) = delete;
    tcp_server(tcp_server &&) = delete;
    tcp_server &operator=(tcp_server &&) = delete;

    ~tcp_server();

    // Set handler which will be triggered when tcp server accepts new connection.
    // @param handler   Handler for the event.
    void set_on_accept(const tcp_event_handler &handler);

    // Set handler which will be triggered when read from tcp connection completes.
    // @param handler   Handler for the event.
    void set_on_read_complete(const tcp_event_handler &handler);

    // Set handler which will be triggered when write to tcp connection completes.
    // @param handler   Handler for the event.
    void set_on_write_complete(const tcp_event_handler &handler);

    // Set handler which will be triggered when tcp socket is closed by opposite host.
    // @param handler   Handler for the event.
    void set_on_closed(const tcp_event_handler &handler);

    // Listen in port.
    // @param port      TCP Port to listen.
    // @param f         TCP socket family, can be IPv4 or IPv6.
    // @param ip        IP to bind.
    void listen(int port, family f, const char *ip=nullptr);

    // Listen in uri.
    // @param path      TCP Unix socket path to listen.
    // @param remove    Whether remove the socket file when it already exists.
    void listen_unix(const std::string &path, bool remove=false);

    // Start server asynchronously.
    void run();

    // Shutdown server synchronously, return when all server threads exit.
    void shutdown();

private:
    // Thread pool shared data.
    tp_shared_data data_;

    // Whether use single acceptor for multiple listening socket.
    bool single_acceptor_;

    // Worker threads.
    thread_pool<iohandler, tp_shared_data *> tp_;

    // Listening threads.
    std::vector<std::unique_ptr<acceptor>> acpts_;
};


class tcp_client final
{
public:
    // Construct tcp client
    // @param iohandler_num     IO thread pool size.
    // @param connector_num     Number of connector.
    // @param external_data     External data pointer.
    explicit tcp_client(int iohandler_num, int connector_num=1, void *external_data=nullptr);

    tcp_client(const tcp_client &) = delete;
    tcp_client &operator=(const tcp_client &) = delete;
    tcp_client(tcp_client &&) = delete;
    tcp_client &operator=(tcp_client &&) = delete;

    ~tcp_client();

    // Set handler which will be triggered when tcp client establishes new connection.
    // @param handler   Handler for the event.
    void set_on_connect(const tcp_event_handler &handler);

    // Set handler which will be triggered when read from tcp connection completes.
    // @param handler   Handler for the event.
    void set_on_read_complete(const tcp_event_handler &handler);

    // Set handler which will be triggered when write to tcp connection completes.
    // @param handler   Handler for the event.
    void set_on_write_complete(const tcp_event_handler &handler);

    // Set handler which will be triggered when tcp socket is closed by opposite host.
    // @param handler   Handler for the event.
    void set_on_closed(const tcp_event_handler &handler);

    // Add target uri to connect.
    // @param ip        Opposite host IP.
    // @param port      Opposite port.
    // @param f         TCP socket family, can be IPv4 or IPv6.
    // @param t         Counts of the uri to add.
    void add(const std::string &ip, int port, family f, int t=1);

    // Add target uri to connect.
    // @param path      TCP Unix socket path to connect.
    // @param t         Counts of the uri to add.
    void add_unix(const std::string &path, int t=1);

    // Start server asynchronously.
    void run();

    // Shutdown client synchronously, return when all server threads exit.
    void shutdown();

private:
    // Thread pool shared data.
    tp_shared_data data_;

    // Worker threads.
    thread_pool<iohandler, tp_shared_data *> tp_;

    // Connecting threads.
    std::vector<std::unique_ptr<connector>> conts_;
};

}   // namespace reactor

}   // namespace cppev

#endif  // tcp.h
