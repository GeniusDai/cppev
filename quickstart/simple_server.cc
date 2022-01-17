#include "cppev/async_logger.h"
#include "cppev/tcp.h"

auto on_accept = [](std::shared_ptr<cppev::nio> iop, cppev::event_loop *evp) -> void {
    iop->wbuf()->put("cppev is an event driven lib");
    cppev::try_write(iop, evp);
    cppev::log::info << "write message to " << iop->fd() << cppev::log::endl;
};

auto on_read_complete = [](std::shared_ptr<cppev::nio> iop, cppev::event_loop *evp) -> void {
    iop->wbuf()->put(iop->rbuf()->get());
    cppev::try_write(iop, evp);
};

int main() {
    cppev::tcp_server server(32);
    server.set_on_accept(on_accept);
    server.set_on_read_complete(on_read_complete);
    server.listen(8888, cppev::family::ipv4);
    server.run();
    return 0;
}