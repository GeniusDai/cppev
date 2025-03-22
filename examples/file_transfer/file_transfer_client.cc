#include <fcntl.h>

#include <mutex>
#include <thread>
#include <unordered_map>

#include "config.h"
#include "cppev/cppev.h"

class fdcache final
{
public:
    void setfd(int conn, int file_descriptor)
    {
        std::unique_lock<std::mutex> lock(lock_);
        hash_[conn] = std::make_shared<cppev::stream>(file_descriptor);
    }

    std::shared_ptr<cppev::stream> getfd(int conn)
    {
        std::unique_lock<std::mutex> lock(lock_);
        return hash_[conn];
    }

private:
    std::unordered_map<int, std::shared_ptr<cppev::stream>> hash_;

    std::mutex lock_;
};

cppev::reactor::tcp_event_handler on_connect =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    iopt->wbuffer().put_string(FILENAME);
    iopt->wbuffer().put_string("\n");
    cppev::reactor::async_write(iopt);
    LOG_INFO << "request file " << FILENAME;

    std::stringstream file_copy_name;
    file_copy_name << FILENAME << "." << iopt->fd() << "." << std::showbase
                   << std::hex << std::this_thread::get_id() << ".copy";
    int fd = open(file_copy_name.str().c_str(), O_WRONLY | O_CREAT | O_APPEND,
                  S_IRWXU);
    if (fd < 0)
    {
        cppev::throw_system_error("open error");
    }
    fdcache *cache =
        reinterpret_cast<fdcache *>(cppev::reactor::external_data(iopt));
    cache->setfd(iopt->fd(), fd);
    LOG_INFO << "create file " << file_copy_name.str();
};

cppev::reactor::tcp_event_handler on_read_complete =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    iopt->read_all();
    fdcache *cache =
        reinterpret_cast<fdcache *>(cppev::reactor::external_data(iopt));
    auto iops = cache->getfd(iopt->fd());
    iops->wbuffer().put_string(iopt->rbuffer().get_string());
    iops->write_all();
    LOG_INFO << "writing chunk to file complete";
};

cppev::reactor::tcp_event_handler on_closed =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{ LOG_INFO << "receiving file complete"; };

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

    LOG_INFO << "main thread exited";

    return 0;
}
