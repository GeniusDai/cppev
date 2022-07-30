/*
 * Simple Client
 *
 * The simple client just echo any message back to the server.
 */

#include "cppev/async_logger.h"
#include "cppev/tcp.h"
#include "config.h"

/*
 * Define Handler
 */
cppev::tcp_event_cb on_connect = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "connect succeed with fd " << iopt->fd() << cppev::log::endl;
};

cppev::tcp_event_cb on_read_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "read message complete" << cppev::log::endl;
    cppev::log::info << "[fd] " << iopt->fd() <<  " | [message] " << iopt->rbuf()->buf() << cppev::log::endl;
    iopt->wbuf()->put(iopt->rbuf()->get());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cppev::tcp::async_write(iopt);
};

cppev::tcp_event_cb on_write_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "write message complete" << cppev::log::endl;
};

/*
 * Start Client
 *
 * Use io-threads to perform the handler and connector-threads to perform the connect operation.
 * Set handlers to the client, then use ipv4/ipv6/unix tcp sockets to perform the stress test.
 */
int main()
{
    cppev::tcp_client client(CLIENT_WORKER_NUM, CONTOR_NUM);
    client.set_on_connect(on_connect);
    client.set_on_read_complete(on_read_complete);
    client.set_on_write_complete(on_write_complete);

    // Please lower the concurrency number if server refused to connect
    // in your OS, especially the unix domain socket.
    client.add(     "127.0.0.1", IPV4_PORT, cppev::family::ipv4, IPV4_CONCURRENCY);
    client.add(     "::1",       IPV6_PORT, cppev::family::ipv6, IPV6_CONCURRENCY);
    client.add_unix(             UNIX_PATH,                      UNIX_CONCURRENCY);

    client.run();
    return 0;
}
