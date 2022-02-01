#include "cppev/event_loop.h"
#include "cppev/nio.h"
#include <vector>

int tcp_ipv4_port = 8889;

int tcp_ipv6_port = 9000;

const char *tcp_unix_path = "./tcp_unix";

cppev::fd_event_cb conn_cb = [](std::shared_ptr<cppev::nio> iop) -> void
{
    cppev::nsocktcp *iops = dynamic_cast<cppev::nsocktcp *>(iop.get());
    iops->read_all();
    cppev::log::info << "tcp --> fd " << iops->fd() << " --> ";
    if (iops->sockfamily() == cppev::family::local)
    {
        cppev::log::info << "[\tunixdomain\t] ";
    }
    else
    {
        auto sock = iops->sockname();
        auto peer = iops->peername();
        cppev::log::info << "[sock: " << std::get<0>(sock) << " " << std::get<1>(sock) << "\t";
        cppev::log::info << "peer: " << std::get<0>(peer) << " " << std::get<1>(peer) << "] ";
    }
    cppev::log::info << " --> " << iops->rbuf()->get() << cppev::log::endl;
    iop->evlp()->fd_remove(iop);
};

cppev::fd_event_cb listen_cb = [](std::shared_ptr<cppev::nio> iop) -> void
{
    cppev::nsocktcp *iopt = dynamic_cast<cppev::nsocktcp *>(iop.get());
    std::vector<std::shared_ptr<cppev::nsocktcp> > vts = iopt->accept(1);
    std::shared_ptr<cppev::nio> conn = std::dynamic_pointer_cast<cppev::nio>(vts[0]);
    iop->evlp()->fd_register(conn, cppev::fd_event::fd_readable, conn_cb, true);
};

void start_server_loop()
{
    remove(tcp_unix_path);
    cppev::event_loop evlp;

    auto tcp_ipv4 = cppev::nio_factory::get_nsocktcp(cppev::family::ipv4);
    auto tcp_ipv6 = cppev::nio_factory::get_nsocktcp(cppev::family::ipv6);
    auto tcp_unix = cppev::nio_factory::get_nsocktcp(cppev::family::local);

    tcp_ipv4->listen(tcp_ipv4_port);
    tcp_ipv6->listen(tcp_ipv6_port);
    tcp_unix->listen_unix(tcp_unix_path);

    evlp.fd_register(tcp_ipv4, cppev::fd_event::fd_readable, listen_cb);
    evlp.fd_register(tcp_ipv6, cppev::fd_event::fd_readable, listen_cb);
    evlp.fd_register(tcp_unix, cppev::fd_event::fd_readable, listen_cb);

    evlp.loop();
}

int main()
{
    start_server_loop();
    return 0;
}