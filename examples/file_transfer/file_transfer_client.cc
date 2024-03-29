#include <thread>
#include <mutex>
#include <unordered_map>
#include <fcntl.h>
#include "config.h"
#include "cppev/cppev.h"

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
    std::unordered_map<int, std::shared_ptr<cppev::nstream>> hash_;

    std::mutex lock_;
};

cppev::reactor::tcp_event_handler on_connect = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    iopt->wbuffer().put_string(FILENAME);
    iopt->wbuffer().put_string("\n");
    iopt->write_all();

    std::string file_copy_name = std::string(FILENAME) + "." + std::to_string(iopt->fd()) + "."
         + std::to_string(cppev::gettid()) + ".copy";
    int fd = open(file_copy_name.c_str(), O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
    if (fd < 0)
    {
        cppev::throw_system_error("open error");
    }
    fdcache *cache = reinterpret_cast<fdcache *>(cppev::reactor::external_data(iopt));
    cache->setfd(iopt->fd() ,fd);
};

cppev::reactor::tcp_event_handler on_read_complete = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    iopt->read_all();
    fdcache *cache = reinterpret_cast<fdcache *>(cppev::reactor::external_data(iopt));
    auto iops = cache->getfd(iopt->fd());
    iops->wbuffer().put_string(iopt->rbuffer().get_string());
    iops->write_all();
    cppev::log::info << "writing chunk to file complete" << cppev::log::endl;
};

cppev::reactor::tcp_event_handler on_closed = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    cppev::log::info << "receiving file complete" << cppev::log::endl;
};

int main(int argc, char **argv)
{
    cppev::thread_block_signal(SIGINT);

    fdcache cache;
    cppev::reactor::tcp_client client(6, 1, &cache);

    client.set_on_connect(on_connect);
    client.set_on_read_complete(on_read_complete);
    client.set_on_closed(on_closed);

    client.add("127.0.0.1", PORT, cppev::family::ipv4, CONCURRENCY);
    client.run();

    cppev::thread_wait_for_signal(SIGINT);

    client.shutdown();

    cppev::log::info << "main thread exited" << cppev::log::endl;

    return 0;
}
