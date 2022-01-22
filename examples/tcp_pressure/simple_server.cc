#include "cppev/async_logger.h"
#include "cppev/tcp.h"

cppev::fd_event_cb on_accept = [](std::shared_ptr<cppev::nio> iop) -> void {
    iop->wbuf()->put("Cppev is a C++ event driven library");
    cppev::async_write(iop);
    cppev::log::info << "write message to " << iop->fd() << cppev::log::endl;
};

cppev::fd_event_cb on_read_complete = [](std::shared_ptr<cppev::nio> iop) -> void {
    iop->wbuf()->put(iop->rbuf()->get());
    cppev::async_write(iop);
};

int main() {
    cppev::tcp_server server(32);
    server.set_on_accept(on_accept);
    server.set_on_read_complete(on_read_complete);
    server.listen(8888, cppev::family::ipv6);
    server.run();
    return 0;
}