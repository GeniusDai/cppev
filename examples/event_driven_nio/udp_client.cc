#include "cppev/nio.h"
#include "cppev/event_loop.h"

int udp_ipv4_port = 8889;

int udp_ipv6_port = 9000;

const char *udp_unix_path = "./udp_unix";

const char *str = "Cppev is a C++ event driven library";

void connect_to_servers() {
    cppev::event_loop evlp;

    auto udp_ipv4 = cppev::nio_factory::get_nsockudp(cppev::family::ipv4);
    auto udp_ipv6 = cppev::nio_factory::get_nsockudp(cppev::family::ipv6);
    auto udp_unix = cppev::nio_factory::get_nsockudp(cppev::family::local);

    udp_ipv4->wbuf()->put(str);
    udp_ipv6->wbuf()->put(str);
    udp_unix->wbuf()->put(str);

    udp_ipv4->send("127.0.0.1", udp_ipv4_port);
    udp_ipv6->send("::1", udp_ipv6_port);

    udp_ipv4->send("127.0.0.1", udp_ipv6_port);     // NOT OK
    udp_ipv6->send("::1", udp_ipv4_port);           // OK IN LINUX

    udp_unix->send(udp_unix_path);

}

int main() {
    connect_to_servers();
    return 0;
}