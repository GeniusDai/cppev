/*
 * Simple Client
 *
 * The simple client just echo any message back to the server.
 */

#include "config.h"
#include "cppev/logger.h"
#include "cppev/tcp.h"

/*
 * Define Handler
 *
 * On the socket connect succeeded : Log the info.
 *
 * On the socket read from sys-buffer completed : Write the message back to the
 * server.
 *
 * On the socket write to sys-buffer completed : Log the info.
 *
 * On the socket closed : Log the info.
 */
cppev::reactor::tcp_event_handler on_connect =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{ LOG_DEBUG_FMT("Fd %d on accept finish", iopt->fd()); };

cppev::reactor::tcp_event_handler on_read_complete =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    std::string message = iopt->rbuffer().get_string();
    LOG_INFO_FMT("Received message : %s", message.c_str());

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    iopt->wbuffer().put_string(message);
    cppev::reactor::async_write(iopt);
    LOG_DEBUG_FMT("Fd %d on read finish", iopt->fd());
};

cppev::reactor::tcp_event_handler on_write_complete =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{ LOG_DEBUG_FMT("Fd %d on write finish", iopt->fd()); };

cppev::reactor::tcp_event_handler on_closed =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{ LOG_DEBUG_FMT("Fd %d on close finish", iopt->fd()); };

/*
 * Start Client
 *
 * Use io-threads to perform the handler and connector-threads to perform the
 * connect operation. Set handlers to the client, then use ipv4/ipv6/unix tcp
 * sockets to perform the stress test.
 */
int main()
{
    cppev::logger::get_instance().set_log_level(cppev::log_level::info);

    cppev::thread_block_signal(SIGINT);

    cppev::reactor::tcp_client client(CLIENT_WORKER_NUM, CONTOR_NUM);
    client.set_on_connect(on_connect);
    client.set_on_read_complete(on_read_complete);
    client.set_on_write_complete(on_write_complete);
    client.set_on_closed(on_closed);

    // Please lower the concurrency number if server refused to connect
    // in your OS, especially the unix domain socket.
    client.add("127.0.0.1", IPV4_PORT, cppev::family::ipv4, IPV4_CONCURRENCY);
    client.add("::1", IPV6_PORT, cppev::family::ipv6, IPV6_CONCURRENCY);
    client.add_unix(UNIX_PATH, UNIX_CONCURRENCY);

    client.run();

    cppev::thread_wait_for_signal(SIGINT);

    client.shutdown();

    LOG_INFO << "main thread exited";

    return 0;
}
