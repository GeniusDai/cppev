#include "cppev/async_logger.h"
#include "cppev/tcp.h"

cppev::tcp_event_cb on_accept = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    iopt->wbuf()->put("Cppev is a C++ event driven library");
    cppev::async_write(iopt);
    cppev::log::info << "Connection " << iopt->fd() << " arrived" << cppev::log::endl;
};

cppev::tcp_event_cb on_read_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    iopt->wbuf()->put(iopt->rbuf()->get());
    cppev::async_write(iopt);
};

int main()
{
    cppev::tcp_server server(32);
    server.set_on_accept(on_accept);
    server.set_on_read_complete(on_read_complete);
    server.listen(8888, cppev::family::ipv6);
    server.run();
    return 0;
}
