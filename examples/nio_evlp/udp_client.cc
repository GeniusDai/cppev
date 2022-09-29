#include "cppev/cppev.h"
#include "config.h"

void send_to_servers()
{
    cppev::event_loop evlp;

    auto udp_ipv4 = cppev::nio_factory::get_nsockudp(cppev::family::ipv4);
    auto udp_ipv6 = cppev::nio_factory::get_nsockudp(cppev::family::ipv6);
    auto udp_unix = cppev::nio_factory::get_nsockudp(cppev::family::local);

    udp_ipv4->wbuf()->produce(MSG, MSG_LEN);
    udp_ipv6->wbuf()->produce(MSG, MSG_LEN);
    udp_unix->wbuf()->produce(MSG, MSG_LEN);

    udp_ipv4->send(         "127.0.0.1"  , UDP_IPV4_PORT    );
    udp_ipv6->send(         "::1"        , UDP_IPV6_PORT    );
    udp_unix->send_unix(    UDP_UNIX_PATH                   );
}

int main()
{
    send_to_servers();
    return 0;
}