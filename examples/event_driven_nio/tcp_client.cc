#include "cppev/event_loop.h"
#include "cppev/nio.h"

int tcp_ipv4_port = 8889;

int tcp_ipv6_port = 9000;

const char *tcp_unix_path = "./tcp_unix";

const char *str = "Cppev is a C++ event driven library";

cppev::fd_event_cb client_cb = [](std::shared_ptr<cppev::nio> iop) -> void {
    cppev::nsocktcp *iopt = dynamic_cast<cppev::nsocktcp *>(iop.get());
    if (!iopt->check_connect()) {
        cppev::log::error << "fd" << iop->fd() << "failed to connect" << cppev::log::endl;
        return;
    }
    iopt->wbuf()->put(str);
    iopt->write_all();
};

void connect_to_servers() {
    cppev::event_loop evlp;

    auto tcp_ipv4 = cppev::nio_factory::get_nsocktcp(cppev::family::ipv4);
    auto tcp_ipv6 = cppev::nio_factory::get_nsocktcp(cppev::family::ipv6);
    auto tcp_unix = cppev::nio_factory::get_nsocktcp(cppev::family::local);

    tcp_ipv4->connect("127.0.0.1", tcp_ipv4_port);
    tcp_ipv6->connect("::1", tcp_ipv6_port);
    tcp_unix->connect_unix(tcp_unix_path);

    evlp.fd_register(tcp_ipv4, cppev::fd_event::fd_writable, client_cb);
    evlp.fd_register(tcp_ipv6, cppev::fd_event::fd_writable, client_cb);
    evlp.fd_register(tcp_unix, cppev::fd_event::fd_writable, client_cb);

    // Cross socket family
    auto tcp_ipv4_to_ipv6 = cppev::nio_factory::get_nsocktcp(cppev::family::ipv4);
    tcp_ipv4_to_ipv6->connect("127.0.0.1", tcp_ipv6_port);
    evlp.fd_register(tcp_ipv4_to_ipv6, cppev::fd_event::fd_writable, client_cb);

    evlp.loop_once();
}

int main() {
    connect_to_servers();
    return 0;
}