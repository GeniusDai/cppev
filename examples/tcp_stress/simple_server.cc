/*
 * Simple Server
 *
 * The simple server just send a message to the client, then echo any message
 * back to the client.
 */

#include "config.h"
#include "cppev/logger.h"
#include "cppev/tcp.h"

/*
 * Define handler
 *
 * On the socket accepted : Put the message to the write buffer, then trying to
 * send it. The async_write here means if the message is very long then would
 * register writable event to do asynchrous write.
 *
 * On the socket read from sys-buffer completed : Retrieve the message from read
 * buffer, then put to the write buffer and trying to send it.
 *
 * On the socket write to sys-buffer completed : Log the info.
 *
 * On the socket closed : Log the info.
 */
cppev::reactor::tcp_event_handler on_accept =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    iopt->wbuffer().put_string("Cppev is a C++ event driven library");
    cppev::reactor::async_write(iopt);
    LOG_DEBUG_FMT("Fd %d on accept finish", iopt->fd());
};

cppev::reactor::tcp_event_handler on_read_complete =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    std::string message = iopt->rbuffer().get_string();
    LOG_INFO_FMT("Received message : %s", message.c_str());

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
 * Start Server
 *
 * Use io-threads to perform the handler and acceptor-threads perform the accept
 * operation. Set handlers to the server, start to listen in port with
 * ipv4/ipv6/unix network layer protocol.
 */
int main()
{
    cppev::logger::get_instance().set_log_level(cppev::log_level::info);

    cppev::thread_block_signal(SIGINT);

    cppev::reactor::tcp_server server(SERVER_WORKER_NUM, SINGLE_ACPT);
    server.set_on_accept(on_accept);
    server.set_on_read_complete(on_read_complete);
    server.set_on_write_complete(on_write_complete);
    server.set_on_closed(on_closed);

    // Create listening thread
    server.listen(IPV4_PORT, cppev::family::ipv4);
    server.listen(IPV6_PORT, cppev::family::ipv6);
    server.listen_unix(UNIX_PATH, true);

    server.run();

    cppev::thread_wait_for_signal(SIGINT);

    server.shutdown();

    LOG_INFO << "main thread exited";

    return 0;
}
