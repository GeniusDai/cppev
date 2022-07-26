/*
 * Simple Client
 *
 * The simple client just echo any message back to the server.
 */

#include "cppev/async_logger.h"
#include "cppev/tcp.h"

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
    cppev::async_write(iopt);
};

cppev::tcp_event_cb on_write_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "write message complete" << cppev::log::endl;
};

/*
 * Concurrency Number
 */
#ifdef __APPLE__
    const int INET = 100;
    const int UNIX = 100;
#else
    const int INET = 8000;
    const int UNIX = 2000;
#endif

/*
 * Start Client
 *
 * Use 32 io-threads to perform the handler and 5 connector-thread to perform the connect operation.
 * Set handlers to the client, then use ipv4/ipv6/unix tcp sockets to perform the stress test.
 */
int main()
{
    cppev::tcp_client client(32, 3);
    client.set_on_connect(on_connect);
    client.set_on_read_complete(on_read_complete);
    client.set_on_write_complete(on_write_complete);

    // Please lower the concurrency number if server refused to connect
    // in your OS, especially the unix domain socket.

    client.add(     "127.0.0.1", 8884, cppev::family::ipv4, INET);
    client.add(     "::1",       8886, cppev::family::ipv6, INET);
    client.add_unix("/tmp/cppev_test.sock",                 UNIX);

    client.run();

    return 0;
}
