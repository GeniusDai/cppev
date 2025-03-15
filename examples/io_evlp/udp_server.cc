#include "config.h"
#include "cppev/cppev.h"

cppev::fd_event_handler binding_socket_callback =
    [](const std::shared_ptr<cppev::io> &iop) -> void
{
    cppev::sockudp *iopu = dynamic_cast<cppev::sockudp *>(iop.get());
    if (iopu == nullptr)
    {
        cppev::throw_logic_error("server bind socket dynamic cast error!");
    }
    auto cli = iopu->recv();
    auto message = iopu->rbuffer().get_string();
    assert(message == std::string(MSG, 10));
    LOG_INFO_FMT("udp bind sock readable --> fd %d --> %s [%d] --> peer: %s %d",
                 iopu->fd(), message.c_str(), message.size(),
                 std::get<0>(cli).c_str(), std::get<1>(cli));
    LOG_INFO << "Whole message is: " << message;
};

void start_server_loop()
{
    cppev::event_loop evlp;

    auto udp_ipv4 = cppev::io_factory::get_sockudp(cppev::family::ipv4);
    auto udp_ipv6 = cppev::io_factory::get_sockudp(cppev::family::ipv6);
    auto udp_unix = cppev::io_factory::get_sockudp(cppev::family::local);

    udp_ipv4->bind(UDP_IPV4_PORT);
    udp_ipv6->bind(UDP_IPV6_PORT);
    udp_unix->bind_unix(UDP_UNIX_PATH, true);

    evlp.fd_register_and_activate(udp_ipv4, cppev::fd_event::fd_readable,
                                  binding_socket_callback);
    evlp.fd_register_and_activate(udp_ipv6, cppev::fd_event::fd_readable,
                                  binding_socket_callback);
    evlp.fd_register_and_activate(udp_unix, cppev::fd_event::fd_readable,
                                  binding_socket_callback);

    evlp.loop_forever();
}

int main()
{
    start_server_loop();
    return 0;
}
