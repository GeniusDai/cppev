#include "config.h"
#include "cppev/cppev.h"

cppev::fd_event_handler accepted_socket_callback =
    [](const std::shared_ptr<cppev::io> &iop) -> void
{
    cppev::socktcp *iops = dynamic_cast<cppev::socktcp *>(iop.get());
    if (iops == nullptr)
    {
        cppev::throw_logic_error("server connected socket dynamic cast error!");
    }
    iops->read_all();
    auto sock = iops->sockname();
    auto peer = iops->peername();
    auto message = iops->rbuffer().get_string();
    assert(message == std::string(MSG, 10));
    LOG_INFO_FMT(
        "tcp connection readable --> fd %d --> %s [%d] --> sock: %s %d | peer: "
        "%s %d",
        iops->fd(), message.c_str(), message.size(), std::get<0>(sock).c_str(),
        std::get<1>(sock), std::get<0>(peer).c_str(), std::get<1>(peer));
    LOG_INFO << "Whole message is: " << message;
    iop->evlp().fd_remove_and_deactivate(iop, cppev::fd_event::fd_readable);
};

cppev::fd_event_handler listening_socket_callback =
    [](const std::shared_ptr<cppev::io> &iop) -> void
{
    cppev::socktcp *iopt = dynamic_cast<cppev::socktcp *>(iop.get());
    if (iopt == nullptr)
    {
        cppev::throw_logic_error("server listening socket dynamic cast error!");
    }
    std::shared_ptr<cppev::io> conn =
        std::dynamic_pointer_cast<cppev::io>(iopt->accept(1).front());
    iop->evlp().fd_register_and_activate(conn, cppev::fd_event::fd_readable,
                                         accepted_socket_callback);
};

void start_server_loop()
{
    cppev::event_loop evlp;

    auto tcp_ipv4 = cppev::io_factory::get_socktcp(cppev::family::ipv4);
    auto tcp_ipv6 = cppev::io_factory::get_socktcp(cppev::family::ipv6);
    auto tcp_unix = cppev::io_factory::get_socktcp(cppev::family::local);

    tcp_ipv4->bind(TCP_IPV4_PORT);
    tcp_ipv6->bind(TCP_IPV6_PORT);
    tcp_unix->bind_unix(TCP_UNIX_PATH, true);

    tcp_ipv4->listen();
    tcp_ipv6->listen();
    tcp_unix->listen();

    evlp.fd_register_and_activate(tcp_ipv4, cppev::fd_event::fd_readable,
                                  listening_socket_callback);
    evlp.fd_register_and_activate(tcp_ipv6, cppev::fd_event::fd_readable,
                                  listening_socket_callback);
    evlp.fd_register_and_activate(tcp_unix, cppev::fd_event::fd_readable,
                                  listening_socket_callback);

    evlp.loop_forever();
}

int main()
{
    start_server_loop();
    return 0;
}
