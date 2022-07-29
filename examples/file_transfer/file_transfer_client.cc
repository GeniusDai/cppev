#include <thread>
#include <fcntl.h>
#include "cppev/async_logger.h"
#include "cppev/nio.h"
#include "cppev/event_loop.h"
#include "cppev/common_utils.h"
#include "config.h"

cppev::fd_event_cb rd_callback = [](std::shared_ptr<cppev::nio> iop) -> void
{
    cppev::log::info << "readable callback" << cppev::log::endl;
    std::shared_ptr<cppev::nsocktcp> iopt = std::dynamic_pointer_cast<cppev::nsocktcp>(iop);
    if (iopt->eof() || iopt->is_reset())
    {
        cppev::log::info << "receive file complete" << cppev::log::endl;
        iop->evlp()->fd_remove(iop, true);
    }
    iopt->read_all();
    std::string file_copy = std::string(file) + "." + std::to_string(cppev::utils::gettid()) + ".copy";
    int fd = open(file_copy.c_str(), O_WRONLY | O_CREAT | O_APPEND, S_IRWXU);
    if (fd < 0)
    {
        cppev::throw_system_error("open error");
    }
    cppev::nstream ios(fd);
    ios.wbuf()->put(iopt->rbuf()->get());
    ios.write_all();
    cppev::log::info << "write chunk to file complete" << cppev::log::endl;
};

cppev::fd_event_cb wr_callback = [](std::shared_ptr<cppev::nio> iop) -> void
{
    cppev::log::info << "writable callback" << cppev::log::endl;
    std::shared_ptr<cppev::nsocktcp> iopt = std::dynamic_pointer_cast<cppev::nsocktcp>(iop);
    if (!iopt->check_connect())
    {
        cppev::throw_system_error("connect error");
    }
    iopt->wbuf()->put(file);
    iopt->wbuf()->put("\n");
    iopt->write_all();
    iop->evlp()->fd_remove(iop, false);
    iop->evlp()->fd_register(iop, cppev::fd_event::fd_readable);
};

void request_file()
{
    std::shared_ptr<cppev::nsocktcp> iopt = cppev::nio_factory::get_nsocktcp(cppev::family::ipv4);
    iopt->connect("127.0.0.1", port);
    cppev::event_loop evlp;
    std::shared_ptr<cppev::nio> iop = std::dynamic_pointer_cast<cppev::nio>(iopt);
    evlp.fd_register(iop, cppev::fd_event::fd_readable, rd_callback, false);
    evlp.fd_register(iop, cppev::fd_event::fd_writable, wr_callback, true);
    evlp.loop();
}

int main(int argc, char **argv)
{
    std::vector<std::thread> thrs;
    for (int i = 0; i < client_concurrency_num; ++i)
    {
        thrs.emplace_back(request_file);
    }
    for (int i = 0; i < client_concurrency_num; ++i)
    {
        thrs[i].join();
    }
    return 0;
}