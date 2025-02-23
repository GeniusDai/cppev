#include "cppev/cppev.h"
#include "config.h"

cppev::fd_event_handler accepted_socket_callback = [](const std::shared_ptr<cppev::nio> &iop) -> void
{
    cppev::nsocktcp *iops = dynamic_cast<cppev::nsocktcp *>(iop.get());
    iops->read_all();
    LOG_INFO << "tcp connection readable --> fd " << iops->fd();
    auto sock = iops->sockname();
    auto peer = iops->peername();
    LOG_INFO << iops->rbuffer().size() << " " << iops->rbuffer().get_string() << " --> "
        << "sock: " << std::get<0>(sock) << " " << std::get<1>(sock) << " | "
        << "peer: " << std::get<0>(peer) << " " << std::get<1>(peer);
    iop->evlp().fd_remove_and_deactivate(iop, cppev::fd_event::fd_readable);
};

cppev::fd_event_handler listening_socket_callback = [](const std::shared_ptr<cppev::nio> &iop) -> void
{
    cppev::nsocktcp *iopt = dynamic_cast<cppev::nsocktcp *>(iop.get());
    std::shared_ptr<cppev::nio> conn = std::dynamic_pointer_cast<cppev::nio>(iopt->accept(1).front());
    iop->evlp().fd_register_and_activate(conn, cppev::fd_event::fd_readable, accepted_socket_callback);
};

void start_server_loop()
{
    cppev::event_loop evlp;

    auto tcp_ipv4 = cppev::nio_factory::get_nsocktcp(cppev::family::ipv4);
    auto tcp_ipv6 = cppev::nio_factory::get_nsocktcp(cppev::family::ipv6);
    auto tcp_unix = cppev::nio_factory::get_nsocktcp(cppev::family::local);

    tcp_ipv4->bind(       TCP_IPV4_PORT   );
    tcp_ipv6->bind(       TCP_IPV6_PORT   );
    tcp_unix->bind_unix(  TCP_UNIX_PATH   , true);

    tcp_ipv4->listen();
    tcp_ipv6->listen();
    tcp_unix->listen();

    evlp.fd_register_and_activate(tcp_ipv4, cppev::fd_event::fd_readable, listening_socket_callback);
    evlp.fd_register_and_activate(tcp_ipv6, cppev::fd_event::fd_readable, listening_socket_callback);
    evlp.fd_register_and_activate(tcp_unix, cppev::fd_event::fd_readable, listening_socket_callback);

    evlp.loop_forever();
}

int main()
{
    start_server_loop();
    return 0;
}
