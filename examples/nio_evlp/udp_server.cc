#include <cstdio>
#include "cppev/nio.h"
#include "cppev/event_loop.h"
#include "config.h"

cppev::fd_event_handler bind_cb = [](std::shared_ptr<cppev::nio> iop) -> void
{
    cppev::nsockudp *iopu = dynamic_cast<cppev::nsockudp *>(iop.get());
    auto cli = iopu->recv();
    cppev::log::info << "udp bind sock readable --> fd " << iopu->fd() << " --> ";
    cppev::log::info << iopu->rbuf()->size() << " " << iopu->rbuf()->get_string() <<  " --> ";
    cppev::log::info << "peer: " << std::get<0>(cli) << " " << std::get<1>(cli) << cppev::log::endl;
};

void start_server_loop()
{
    remove(udp_unix_path);
    cppev::event_loop evlp;

    auto udp_ipv4 = cppev::nio_factory::get_nsockudp(cppev::family::ipv4);
    auto udp_ipv6 = cppev::nio_factory::get_nsockudp(cppev::family::ipv6);
    auto udp_unix = cppev::nio_factory::get_nsockudp(cppev::family::local);

    udp_ipv4->bind(         udp_ipv4_port   );
    udp_ipv6->bind(         udp_ipv6_port   );
    udp_unix->bind_unix(    udp_unix_path   );

    evlp.fd_register(udp_ipv4, cppev::fd_event::fd_readable, bind_cb);
    evlp.fd_register(udp_ipv6, cppev::fd_event::fd_readable, bind_cb);
    evlp.fd_register(udp_unix, cppev::fd_event::fd_readable, bind_cb);

    evlp.loop();
}


int main()
{
    start_server_loop();
    return 0;
}
