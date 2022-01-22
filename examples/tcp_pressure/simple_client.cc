#include "cppev/async_logger.h"
#include "cppev/tcp.h"

cppev::fd_event_cb on_read_complete = [](std::shared_ptr<cppev::nio> iop) -> void {
    cppev::log::info << "receive message --> " << iop->rbuf()->buf() << cppev::log::endl;
    iop->wbuf()->put(iop->rbuf()->get());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cppev::async_write(iop);
};

cppev::fd_event_cb on_write_complete = [](std::shared_ptr<cppev::nio> iop) -> void {
    cppev::log::info << "write message complete" << cppev::log::endl;
};

int main() {
    cppev::tcp_client client(32);
    client.set_on_read_complete(on_read_complete);
    client.set_on_write_complete(on_write_complete);
    client.add("127.0.0.1", 8888, cppev::family::ipv4, 10000);
    client.add("::1", 8888, cppev::family::ipv6, 10000);
    client.run();
    return 0;
}