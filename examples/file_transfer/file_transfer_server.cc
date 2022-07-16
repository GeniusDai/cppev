#include "cppev/async_logger.h"
#include "cppev/tcp.h"
#include <fcntl.h>
#include <thread>

const int chunk_size = 10 * 1024 * 1024;    // 10MB

const int port = 8891;

cppev::tcp_event_cb on_read_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "callback : on_read_complete" << cppev::log::endl;
    std::string file = iopt->rbuf()->get(-1, false);
    if (file[file.size()-1] != '\n')
    {
        return;
    }
    iopt->rbuf()->clear();
    file = file.substr(0, file.size()-1);
    cppev::log::info << "client request file : " << file << cppev::log::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    int fd = open(file.c_str(), O_RDONLY);
    std::shared_ptr<cppev::nstream> iops(new cppev::nstream(fd));
    iops->read_all(chunk_size);
    iopt->wbuf()->put(iops->rbuf()->buf());
    cppev::async_write(iopt);
    cppev::log::info << "transfer file complete" << cppev::log::endl;
};

cppev::tcp_event_cb on_write_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "callback : on_write_complete" << cppev::log::endl;
    cppev::safely_close(iopt);
};

int main()
{
    cppev::tcp_server server(2);
    server.set_on_read_complete(on_read_complete);
    server.set_on_write_complete(on_write_complete);
    server.listen(port, cppev::family::ipv4);
    server.run();
    return 0;
}