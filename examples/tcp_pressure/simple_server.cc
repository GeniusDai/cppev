/*
 * Simple Server
 *
 * The simple server just send a message to the client, then echo any message back to the client.
 */

#include "cppev/async_logger.h"
#include "cppev/tcp.h"

/*
 * Define handler
 *
 * On the socket accepted, put the message to the write buffer, then trying to send it.
 * The async_write here means if the message is very long then would register writable
 * event to do asynchrous write.
 *
 * On the socket sys-buffer read completed, retrieve the message from read buffer, then
 * put to the write buffer and trying to send it.
 */
cppev::tcp_event_cb on_accept = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    iopt->wbuf()->put("Cppev is a C++ event driven library");
    cppev::async_write(iopt);
    cppev::log::info << "Connection " << iopt->fd() << " arrived" << cppev::log::endl;
};

cppev::tcp_event_cb on_read_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    iopt->wbuf()->put(iopt->rbuf()->get());
    cppev::async_write(iopt);
};

/*
 * Start Server
 *
 * Use 32 io-threads to perform the handler, also implicitly there will one thread perform
 * the accept operation. Set handlers to the server, start to listen in port with ipv6 network
 * layer protocol.
*/
int main()
{
    cppev::tcp_server server(32);
    server.set_on_accept(on_accept);
    server.set_on_read_complete(on_read_complete);
    server.listen(8884, cppev::family::ipv4);
    server.listen(8886, cppev::family::ipv6);
    server.listen_unix("/tmp/cppev_test.sock");
    server.run();
    return 0;
}
