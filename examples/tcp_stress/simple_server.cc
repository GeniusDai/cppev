/*
 * Simple Server
 *
 * The simple server just send a message to the client, then echo any message back to the client.
 */

#include <cstdlib>
#include "cppev/async_logger.h"
#include "cppev/tcp.h"
#include "config.h"

/*
 * Define handler
 *
 * On the socket accepted, put the message to the write buffer, then trying to send it.
 * The async_write here means if the message is very long then would register writable
 * event to do asynchrous write.
 *
 * On the socket read from sys-buffer completed, retrieve the message from read buffer, then
 * put to the write buffer and trying to send it.
 * 
 * On the socket closed, log the info.
 */
cppev::reactor::tcp_event_handler on_accept = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    iopt->wbuf()->put_string("Cppev is a C++ event driven library");
    cppev::reactor::async_write(iopt);
    cppev::log::info << "Connection " << iopt->fd() << " arrived" << cppev::log::endl;
};

cppev::reactor::tcp_event_handler on_read_complete = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    iopt->wbuf()->put_string(iopt->rbuf()->get_string());
    cppev::reactor::async_write(iopt);
};

cppev::reactor::tcp_event_handler on_closed = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    cppev::log::info << "Connection " << iopt->fd() << " closed by opposite host" << cppev::log::endl;
};

/*
 * Start Server
 *
 * Use io-threads to perform the handler and acceptor-threads perform the accept operation.
 * Set handlers to the server, start to listen in port with ipv4/ipv6/unix network layer protocol.
 */
int main()
{
    cppev::reactor::tcp_server server(SERVER_WORKER_NUM);
    server.set_on_accept(on_accept);
    server.set_on_read_complete(on_read_complete);
    server.set_on_closed(on_closed);

    // Create listening thread
    server.listen(     IPV4_PORT, cppev::family::ipv4);
    server.listen(     IPV6_PORT, cppev::family::ipv6);
    remove(UNIX_PATH);
    server.listen_unix(UNIX_PATH);

    server.run();
    return 0;
}
