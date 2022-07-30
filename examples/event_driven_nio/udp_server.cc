#include <cstdio>
#include "cppev/nio.h"
#include "cppev/event_loop.h"
#include "config.h"

cppev::fd_event_cb udp_cb = [](std::shared_ptr<cppev::nio> iop) -> void
{
    cppev::nsockudp *ioup = dynamic_cast<cppev::nsockudp *>(iop.get());
    cppev::log::info << "udp --> fd " << iop->fd() << " --> ";
    switch (ioup->sockfamily())
    {
    case cppev::family::ipv4 :
    {
        auto cli = ioup->recv();
        cppev::log::info << "[" << std::get<0>(cli) << " " << std::get<1>(cli)
            << " ipv4] --> ";
        break;
    }
    case cppev::family::ipv6 :
    {
        auto cli = ioup->recv();
        cppev::log::info << "[" << std::get<0>(cli) << " " << std::get<1>(cli)
            << " ipv6] --> ";
        break;
    }
    case cppev::family::local :
    {
        ioup->recv_unix();
        cppev::log::info << "[\tunix-domain\t] --> ";
        break;
    }
    }
    cppev::log::info << ioup->rbuf()->rawbuf() << cppev::log::endl;
};

void start_event_loop()
{
    remove(udp_unix_path);
    cppev::event_loop evlp;

    auto udp_ipv4 = cppev::nio_factory::get_nsockudp(cppev::family::ipv4);
    auto udp_ipv6 = cppev::nio_factory::get_nsockudp(cppev::family::ipv6);
    auto udp_unix = cppev::nio_factory::get_nsockudp(cppev::family::local);

    udp_ipv4->bind(udp_ipv4_port);
    udp_ipv6->bind(udp_ipv6_port);
    udp_unix->bind_unix(udp_unix_path);

    evlp.fd_register(udp_ipv4, cppev::fd_event::fd_readable, udp_cb);
    evlp.fd_register(udp_ipv6, cppev::fd_event::fd_readable, udp_cb);
    evlp.fd_register(udp_unix, cppev::fd_event::fd_readable, udp_cb);

    evlp.loop();
}


int main()
{
    start_event_loop();
    return 0;
}
