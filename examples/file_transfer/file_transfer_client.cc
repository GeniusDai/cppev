#include <thread>
#include <mutex>
#include <unordered_map>
#include <fcntl.h>
#include "cppev/async_logger.h"
#include "cppev/tcp.h"
#include "cppev/event_loop.h"
#include "cppev/common_utils.h"
#include "config.h"

class fdcache final
{
public:
    void setfd(int conn, int file_descriptor)
    {
        std::unique_lock<std::mutex> lock(lock_);
        hash_[conn] = std::make_shared<cppev::nstream>(file_descriptor);
    }

    std::shared_ptr<cppev::nstream> getfd(int conn)
    {
        std::unique_lock<std::mutex> lock(lock_);
        return hash_[conn];
    }

private:
    std::unordered_map<int, std::shared_ptr<cppev::nstream> > hash_;

    std::mutex lock_;
};

cppev::tcp_event_cb on_connect = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    iopt->wbuf()->put_string(file);
    iopt->wbuf()->put_string("\n");
    iopt->write_all();

    std::string file_copy_name = std::string(file) + "." + std::to_string(iopt->fd()) + "."
         + std::to_string(cppev::utils::gettid()) + ".copy";
    int fd = open(file_copy_name.c_str(), O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
    if (fd < 0)
    {
        cppev::throw_system_error("open error");
    }
    fdcache *ptrcache = reinterpret_cast<fdcache *>(cppev::tcp::reactor_external_data(iopt));
    ptrcache->setfd(iopt->fd() ,fd);
};

cppev::tcp_event_cb on_read_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    iopt->read_all();
    fdcache *ptrcache = reinterpret_cast<fdcache *>(cppev::tcp::reactor_external_data(iopt));
    auto ios = ptrcache->getfd(iopt->fd());
    ios->wbuf()->put_string(iopt->rbuf()->get_string());
    ios->write_all();
    cppev::log::info << "write chunk to file complete" << cppev::log::endl;
};

cppev::tcp_event_cb on_closed = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "receive file complete" << cppev::log::endl;
};

int main(int argc, char **argv)
{
    fdcache cache;
    cppev::tcp_client client(6, 1, &cache);

    client.set_on_connect(on_connect);
    client.set_on_read_complete(on_read_complete);
    client.set_on_closed(on_closed);

    client.add("127.0.0.1", port, cppev::family::ipv4, client_num);
    client.run();
    return 0;
}