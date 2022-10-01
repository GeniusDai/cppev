/*
 * Simple Client
 *
 * The simple client just echo any message back to the server.
 */

#include "config.h"
#include "cppev/cppev.h"

/*
 * Define Handler
 *
 * On the socket connect succeeded : Log the info.
 *
 * On the socket read from sys-buffer completed : Write the message back to the server.
 *
 * On the socket write to sys-buffer completed : Log the info.
 *
 * On the socket closed : Log the info.
 */
cppev::reactor::tcp_event_handler on_connect = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    cppev::log::info << "connect succeed with fd " << iopt->fd() << cppev::log::endl;
};

cppev::reactor::tcp_event_handler on_read_complete = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    cppev::log::info << "[fd] " << iopt->fd() << " | [callback] read_complete" << cppev::log::endl;
    cppev::log::info << "[fd] " << iopt->fd() << " | [message] " << iopt->rbuf()->rawbuf() << cppev::log::endl;
    iopt->wbuf()->put_string(iopt->rbuf()->get_string());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cppev::reactor::async_write(iopt);
};

cppev::reactor::tcp_event_handler on_write_complete = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    cppev::log::info << "[fd] " << iopt->fd() << " | [callback] write_complete" << cppev::log::endl;
};

cppev::reactor::tcp_event_handler on_closed = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    cppev::log::info << "Connection " << iopt->fd() << " closed by opposite host" << cppev::log::endl;
};

/*
 * Start Client
 *
 * Use io-threads to perform the handler and connector-threads to perform the connect operation.
 * Set handlers to the client, then use ipv4/ipv6/unix tcp sockets to perform the stress test.
 */
int main()
{
    cppev::reactor::tcp_client client(CLIENT_WORKER_NUM, CONTOR_NUM);
    client.set_on_connect(on_connect);
    client.set_on_read_complete(on_read_complete);
    client.set_on_write_complete(on_write_complete);
    client.set_on_closed(on_closed);

    // Please lower the concurrency number if server refused to connect
    // in your OS, especially the unix domain socket.
    client.add(     "127.0.0.1", IPV4_PORT, cppev::family::ipv4, IPV4_CONCURRENCY);
    client.add(     "::1",       IPV6_PORT, cppev::family::ipv6, IPV6_CONCURRENCY);
    client.add_unix(             UNIX_PATH,                      UNIX_CONCURRENCY);

    client.run();
    return 0;
}
