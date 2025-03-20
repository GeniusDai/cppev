#include <fcntl.h>

#include <thread>
#include <unordered_map>

#include "config.h"
#include "cppev/logger.h"
#include "cppev/tcp.h"

class filecache final
{
public:
    const cppev::buffer *lazyload(const std::string &filename)
    {
        std::unique_lock<std::mutex> lock(lock_);
        if (hash_.count(filename) != 0)
        {
            return &(hash_[filename]->rbuffer());
        }
        LOG_INFO << "start loading file";
        int fd = open(filename.c_str(), O_RDONLY);
        std::shared_ptr<cppev::stream> iops =
            std::make_shared<cppev::stream>(fd);
        iops->read_all(CHUNK_SIZE);
        close(fd);
        hash_[filename] = iops;
        LOG_INFO << "finish loading file";
        return &(iops->rbuffer());
    }

private:
    std::mutex lock_;

    std::unordered_map<std::string, std::shared_ptr<cppev::stream>> hash_;
};

cppev::reactor::tcp_event_handler on_read_complete =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    LOG_INFO << "start callback : on_read_complete";
    std::string filename = iopt->rbuffer().get_string(-1, false);
    if (filename[filename.size() - 1] != '\n')
    {
        return;
    }
    iopt->rbuffer().clear();
    filename = filename.substr(0, filename.size() - 1);
    LOG_INFO << "client request file : " << filename;

    const cppev::buffer *bf =
        reinterpret_cast<filecache *>(cppev::reactor::external_data(iopt))
            ->lazyload(filename);

    iopt->wbuffer().put_string(bf->data(), bf->size());
    cppev::reactor::async_write(iopt);
    LOG_INFO << "end callback : on_read_complete";
};

cppev::reactor::tcp_event_handler on_write_complete =
    [](const std::shared_ptr<cppev::socktcp> &iopt) -> void
{
    LOG_INFO << "start callback : on_write_complete";
    cppev::reactor::safely_close(iopt);
    LOG_INFO << "end callback : on_write_complete";
};

int main()
{
    cppev::thread_block_signal(SIGINT);

    filecache cache;
    cppev::reactor::tcp_server server(3, false, &cache);
    server.set_on_read_complete(on_read_complete);
    server.set_on_write_complete(on_write_complete);
    server.listen(PORT, cppev::family::ipv4);
    server.run();

    cppev::thread_wait_for_signal(SIGINT);

    server.shutdown();

    LOG_INFO << "main thread exited";

    return 0;
}
