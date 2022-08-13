#include "cppev/nio.h"
#include "cppev/event_loop.h"
#include "config.h"

void send_to_servers()
{
    cppev::event_loop evlp;

    auto udp_ipv4 = cppev::nio_factory::get_nsockudp(cppev::family::ipv4);
    auto udp_ipv6 = cppev::nio_factory::get_nsockudp(cppev::family::ipv6);
    auto udp_unix = cppev::nio_factory::get_nsockudp(cppev::family::local);

    udp_ipv4->wbuf()->produce(str, len);
    udp_ipv6->wbuf()->produce(str, len);
    udp_unix->wbuf()->produce(str, len);

    udp_ipv4->send(         "127.0.0.1"  , udp_ipv4_port    );
    udp_ipv6->send(         "::1"        , udp_ipv6_port    );
    udp_unix->send_unix(    udp_unix_path                   );
}

int main()
{
    send_to_servers();
    return 0;
}