#include "config.h"
#include "cppev/cppev.h"

cppev::fd_event_handler connecting_socket_callback =
    [](const std::shared_ptr<cppev::io> &iop) -> void
{
    cppev::socktcp *iopt = dynamic_cast<cppev::socktcp *>(iop.get());
    if (iopt == nullptr)
    {
        cppev::throw_logic_error("client connect socket dynamic cast error!");
    }
    if (!iopt->check_connect())
    {
        LOG_ERROR << "fd " << iop->fd() << " failed to connect";
        return;
    }
    iopt->wbuffer().put_string(MSG, MSG_LEN);
    iopt->write_all();
};

void connect_to_servers()
{
    cppev::event_loop evlp;

    auto tcp_ipv4 = cppev::io_factory::get_socktcp(cppev::family::ipv4);
    auto tcp_ipv6 = cppev::io_factory::get_socktcp(cppev::family::ipv6);
    auto tcp_unix = cppev::io_factory::get_socktcp(cppev::family::local);
    auto tcp_ipv4_to_ipv6 = cppev::io_factory::get_socktcp(cppev::family::ipv4);

    tcp_ipv4->connect("127.0.0.1", TCP_IPV4_PORT);
    tcp_ipv6->connect("::1", TCP_IPV6_PORT);
    tcp_unix->connect_unix(TCP_UNIX_PATH);

    evlp.fd_register_and_activate(tcp_ipv4, cppev::fd_event::fd_writable,
                                  connecting_socket_callback);
    evlp.fd_register_and_activate(tcp_ipv6, cppev::fd_event::fd_writable,
                                  connecting_socket_callback);
    evlp.fd_register_and_activate(tcp_unix, cppev::fd_event::fd_writable,
                                  connecting_socket_callback);

    // Connection is writable when second tcp shake hand is ok
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    evlp.loop_once();
}

int main()
{
    connect_to_servers();
    return 0;
}
