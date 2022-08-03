#include <thread>
#include <unordered_map>
#include <fcntl.h>
#include "cppev/async_logger.h"
#include "cppev/tcp.h"
#include "config.h"

class filecache final
{
public:
    cppev::buffer *lazyload(const std::string &filename) noexcept
    {
        std::unique_lock<std::mutex> lock;
        if (hash_.count(filename) != 0)
        {
            return hash_[filename]->rbuf();
        }
        int fd = open(file, O_RDONLY);
        std::shared_ptr<cppev::nstream> iops = std::make_shared<cppev::nstream>(fd);
        iops->read_all(chunk_size);
        close(fd);
        hash_[filename] = iops;
        return iops->rbuf();
    }

private:
    std::mutex lock_;

    std::unordered_map<std::string, std::shared_ptr<cppev::nstream> > hash_;
};

cppev::tcp_event_handler on_read_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "callback : on_read_complete" << cppev::log::endl;
    std::string filename = iopt->rbuf()->get_string(-1, false);
    if (filename[filename.size()-1] != '\n')
    {
        return;
    }
    iopt->rbuf()->clear();
    filename = filename.substr(0, filename.size()-1);
    cppev::log::info << "client request file : " << filename << cppev::log::endl;

    filecache *fc =reinterpret_cast<filecache *>(cppev::tcp::reactor_external_data(iopt));
    cppev::buffer *bf = fc->lazyload(filename);

    iopt->wbuf()->produce(bf->rawbuf(), bf->size());
    cppev::tcp::async_write(iopt);
    cppev::log::info << "transfer file complete" << cppev::log::endl;
};

cppev::tcp_event_handler on_write_complete = [](std::shared_ptr<cppev::nsocktcp> iopt) -> void
{
    cppev::log::info << "callback : on_write_complete" << cppev::log::endl;
    cppev::tcp::safely_close(iopt);
};

int main()
{
    filecache cache;
    cppev::tcp_server server(2, &cache);
    server.set_on_read_complete(on_read_complete);
    server.set_on_write_complete(on_write_complete);
    server.listen(port, cppev::family::ipv4);
    server.run();
    return 0;
}