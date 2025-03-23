#ifndef _cppev_tcp_h_6C0224787A17_
#define _cppev_tcp_h_6C0224787A17_

#include <signal.h>

#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <random>
#include <type_traits>
#include <vector>

#include "cppev/common.h"
#include "cppev/event_loop.h"
#include "cppev/io.h"
#include "cppev/runnable.h"
#include "cppev/thread_pool.h"

/*
    Q1: The architecture(Reactor-Impl)?
    A1: 1) Support tcp's client(web-crawler) and server.
        2) Using multi-threading(N+M), each thread works with an specific
           io-multiplexing.
        3) N threads deal with the syn_sent / listening socket, M
           threads(thread-pool) deal with the connected socket.
    Q2: The comparation of other implementation?
    A2: 1) Compared with "one or two io-multiplexing"(Reactor-Impl1).
           Reactor-Impl1 distributes sockets to the thread-pool when
           sockets are readable / writable, thread-pool registers sockets
           back after reading / writing. Assume for Linux tcp server
           Reactor-Impl1 lower the performance due to the epolls' RB-tree
           size expansion and being operated by multi-threads. Also why
           cannot the polling task becomes too heavy for only one thread?
           Although Reactor-Impl's coding is more complex.
        2) Compared with "nginx"(Reactor-Impl2). Reactor-Impl2 doesn't
           treat listening socket's event as higher priority, but
           Reactor-Impl does.
 */

namespace cppev
{

namespace reactor
{

// Callback function type.
using tcp_event_handler = std::function<void(const std::shared_ptr<socktcp> &)>;

// Async write data in write buffer.
CPPEV_PUBLIC void async_write(const std::shared_ptr<socktcp> &iopt);

// Safely close tcp socket.
CPPEV_PUBLIC void safely_close(const std::shared_ptr<socktcp> &iopt);

// Get external data of reactor server and client.
CPPEV_PUBLIC void *external_data(const std::shared_ptr<socktcp> &iopt);

struct CPPEV_PRIVATE host_hash
{
    size_t operator()(const std::tuple<std::string, int, family> &h) const;
};

// Data used for event loop initialization.
struct CPPEV_PRIVATE data_storage final
{
private:
    friend class tcp_common;

    // Idle function for callback.
    static const tcp_event_handler idle_handler;

public:
    // All the five callbacks will be executed by worker thread.
    explicit data_storage(void *external_data_ptr);

    data_storage(const data_storage &) = delete;
    data_storage &operator=(const data_storage &) = delete;
    data_storage(data_storage &&) = delete;
    data_storage &operator=(data_storage &&) = delete;

    ~data_storage();

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

class CPPEV_PRIVATE iohandler final : public runnable
{
public:
    using init_checker = std::function<bool(const std::shared_ptr<io> &iop)>;

    explicit iohandler(data_storage *data);

    iohandler(const iohandler &) = delete;
    iohandler &operator=(const iohandler &) = delete;
    iohandler(iohandler &&) = delete;
    iohandler &operator=(iohandler &&) = delete;

    ~iohandler();

    // Connected socket that has been registered to thread pool is readable.
    static void on_readable(const std::shared_ptr<io> &iop);

    // Connected socket that has been registered to thread pool is writable.
    static void on_writable(const std::shared_ptr<io> &iop);

    // Connecting socket is writable, registered by listening / connecting
    // thread and will be executed by one thread of the pool to do the
    // check and init jobs.
    static void on_conn_establish(const std::shared_ptr<io> &iop,
                                  init_checker checker,
                                  tcp_event_handler handler);

    // Get event loop.
    event_loop &evlp();

    // Run io handling.
    void run_without_exception_handling();

    // Run with exception handling.
    void run_impl() override;

    // Shutdown io eventloop.
    void shutdown();

private:
    // Event loop.
    event_loop evlp_;
};

class CPPEV_PRIVATE acceptor final : public runnable
{
public:
    explicit acceptor(data_storage *data);

    acceptor(const acceptor &) = delete;
    acceptor &operator=(const acceptor &) = delete;
    acceptor(acceptor &&) = delete;
    acceptor &operator=(acceptor &&) = delete;

    ~acceptor();

    // Listening socket is readable, indicating new client arrives, this
    // callback will be executed by accept thread to accept connection and
    // assign connection to thread pool.
    static void on_acpt_readable(const std::shared_ptr<io> &iop);

    // Register readable to event loop and start loop.
    void run_without_exception_handling();

    // Run with exception handling.
    void run_impl() override;

    // Specify listening socket's port and family.
    // Should be called before subthread runs.
    void listen(int port, family f, const char *ip);

    // Specify unix domain listening socket's path.
    // Should be called before subthread runs.
    void listen_unix(const std::string &path, bool remove);

    // Shutdown io eventloop.
    // Thread safe.
    void shutdown();

private:
    // Event loop.
    event_loop evlp_;

    // Listening socket.
    std::vector<std::shared_ptr<socktcp>> socks_;
};

class CPPEV_PRIVATE connector final : public runnable
{
public:
    explicit connector(data_storage *data);

    connector(const connector &) = delete;
    connector &operator=(const connector &) = delete;
    connector(connector &&) = delete;
    connector &operator=(connector &&) = delete;

    ~connector();

    // Pipe fd is readable, indicating new task added, this callback will be
    // executed by connect thread to execute the connection task and assign
    // connection to thread pool.
    static void on_pipe_readable(const std::shared_ptr<io> &iop);

    // Register readable to event loop and start loop.
    void run_without_exception_handling();

    // Run with exception handling.
    void run_impl() override;

    // Add connection task (ip, port, family).
    // Thread safe.
    void add(const std::string &ip, int port, family f, int t);

    // Shutdown io eventloop.
    // Thread safe.
    void shutdown();

private:
    // Event loop.
    event_loop evlp_;

    // Thread safe of "adding hosts" * N and "consume hosts".
    std::mutex lock_;

    // Pipe write end.
    std::shared_ptr<stream> wrp_;

    // Pipe read end.
    std::shared_ptr<stream> rdp_;

    // Hosts waiting for connecting.
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash>
        hosts_;

    // Hosts failed in the connect syscall or SO_ERROR check.
    std::unordered_map<std::tuple<std::string, int, family>, int, host_hash>
        failures_;
};

class CPPEV_INTERNAL tcp_common
{
public:
    // Contruct common data structures for tcp server and client.
    explicit tcp_common(int iohandler_num, void *external_data);

    tcp_common(const tcp_common &) = delete;
    tcp_common &operator=(const tcp_common &) = delete;
    tcp_common(tcp_common &&) = delete;
    tcp_common &operator=(tcp_common &&) = delete;

    virtual ~tcp_common();

    // Set handler which will be triggered when read from tcp connection
    // completes.
    // @param handler   Handler for the event.
    void set_on_read_complete(const tcp_event_handler &handler);

    // Set handler which will be triggered when write to tcp connection
    // completes.
    // @param handler   Handler for the event.
    void set_on_write_complete(const tcp_event_handler &handler);

    // Set handler which will be triggered when tcp socket is closed by opposite
    // host.
    // @param handler   Handler for the event.
    void set_on_closed(const tcp_event_handler &handler);

protected:
    template <typename R1>
    void run(std::vector<std::unique_ptr<R1>> &rpv)
    {
        ignore_signal(SIGPIPE);
        tp_.run();
        for (auto &rp : rpv)
        {
            rp->run();
        }
    }

    template <typename R1>
    void shutdown(std::vector<std::unique_ptr<R1>> &rpv)
    {
        for (auto &rp : rpv)
        {
            rp->shutdown();
        }
        for (auto &rp : rpv)
        {
            rp->join();
        }

        for (auto &thr : tp_)
        {
            thr.shutdown();
        }
        for (auto &thr : tp_)
        {
            thr.join();
        }
    }

    // Thread pool shared data.
    data_storage data_;

    // Worker threads.
    thread_pool<iohandler, data_storage *> tp_;
};

class CPPEV_PUBLIC tcp_server final : public tcp_common
{
public:
    // Construct tcp server
    // @param iohandler_num      IO thread pool size.
    // @param single_acceptor    Whether using one acceptor for all listening
    // socket.
    // @param external_data      External data pointer.
    explicit tcp_server(int iohandler_num, bool single_acceptor = true,
                        void *external_data = nullptr);

    ~tcp_server();

    // Set handler which will be triggered when tcp server accepts new
    // connection.
    // @param handler   Handler for the event.
    void set_on_accept(const tcp_event_handler &handler);

    // Listen in port.
    // Can be called only before run().
    // @param port      TCP Port to listen.
    // @param f         TCP socket family, can be IPv4 or IPv6.
    // @param ip        IP to bind.
    void listen(int port, family f, const char *ip = nullptr);

    // Listen in uri.
    // Can be called only before run().
    // @param path      TCP Unix socket path to listen.
    // @param remove    Whether remove the socket file when it already exists.
    void listen_unix(const std::string &path, bool remove = false);

    // Start server asynchronously.
    void run();

    // Shutdown server synchronously, return when all server threads exit.
    void shutdown();

private:
    // Whether use single acceptor for multiple listening socket.
    bool single_acceptor_;

    // Listening threads.
    std::vector<std::unique_ptr<acceptor>> acpts_;
};

class CPPEV_PUBLIC tcp_client final : public tcp_common
{
public:
    // Construct tcp client
    // @param iohandler_num     IO thread pool size.
    // @param connector_num     Number of connector.
    // @param external_data     External data pointer.
    explicit tcp_client(int iohandler_num, int connector_num = 1,
                        void *external_data = nullptr);

    ~tcp_client();

    // Set handler which will be triggered when tcp client establishes new
    // connection.
    // @param handler   Handler for the event.
    void set_on_connect(const tcp_event_handler &handler);

    // Add target uri to connect.
    // Can be called before or after run().
    // @param ip        Opposite host IP.
    // @param port      Opposite port.
    // @param f         TCP socket family, can be IPv4 or IPv6.
    // @param t         Counts of the uri to add.
    void add(const std::string &ip, int port, family f, int t = 1);

    // Add target uri to connect.
    // Can be called before or after run().
    // @param path      TCP Unix socket path to connect.
    // @param t         Counts of the uri to add.
    void add_unix(const std::string &path, int t = 1);

    // Start client asynchronously.
    void run();

    // Shutdown client synchronously, return when all client threads exit.
    void shutdown();

private:
    // Connecting threads.
    std::vector<std::unique_ptr<connector>> conts_;
};

}  // namespace reactor

}  // namespace cppev

#endif  // tcp.h
