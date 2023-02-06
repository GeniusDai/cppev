#include <thread>
#include <unordered_map>
#include <fcntl.h>
#include "config.h"
#include "cppev/cppev.h"

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
        cppev::log::info << "start loading file" << cppev::log::endl;
        int fd = open(filename.c_str(), O_RDONLY);
        std::shared_ptr<cppev::nstream> iops = std::make_shared<cppev::nstream>(fd);
        iops->read_all(CHUNK_SIZE);
        close(fd);
        hash_[filename] = iops;
        cppev::log::info << "finish loading file" << cppev::log::endl;
        return &(iops->rbuffer());
    }

private:
    std::mutex lock_;

    std::unordered_map<std::string, std::shared_ptr<cppev::nstream>> hash_;
};

cppev::reactor::tcp_event_handler on_read_complete = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    cppev::log::info << "start callback : on_read_complete" << cppev::log::endl;
    std::string filename = iopt->rbuffer().get_string(-1, false);
    if (filename[filename.size()-1] != '\n')
    {
        return;
    }
    iopt->rbuffer().clear();
    filename = filename.substr(0, filename.size()-1);
    cppev::log::info << "client request file : " << filename << cppev::log::endl;

    const cppev::buffer *bf = reinterpret_cast<filecache *>(cppev::reactor::external_data(iopt))->lazyload(filename);

    iopt->wbuffer().produce(bf->rawbuf(), bf->size());
    cppev::reactor::async_write(iopt);
    cppev::log::info << "end callback : on_read_complete" << cppev::log::endl;
};

cppev::reactor::tcp_event_handler on_write_complete = [](const std::shared_ptr<cppev::nsocktcp> &iopt) -> void
{
    cppev::log::info << "start callback : on_write_complete" << cppev::log::endl;
    cppev::reactor::safely_close(iopt);
    cppev::log::info << "end callback : on_write_complete" << cppev::log::endl;
};

int main()
{
    filecache cache;
    cppev::reactor::tcp_server server(3, &cache);
    server.set_on_read_complete(on_read_complete);
    server.set_on_write_complete(on_write_complete);
    server.listen(PORT, cppev::family::ipv4);
    server.run();
    return 0;
}
