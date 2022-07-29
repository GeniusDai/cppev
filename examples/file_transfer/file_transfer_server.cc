#include <thread>
#include <fcntl.h>
#include "cppev/async_logger.h"
#include "cppev/tcp.h"
#include "config.h"

std::mutex lock;
std::shared_ptr<cppev::nstream> iops;

void load_to_memory()
{
    int fd = open(file, O_RDONLY);
    iops = std::make_shared<cppev::nstream>(fd);
    iops->read_all(chunk_size);
    close(fd);
}

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
    if (iops == nullptr)
    {
        std::unique_lock<std::mutex> _(lock);
        if (iops == nullptr)
        {
            load_to_memory();
        }
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));

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