#include "cppev/async_logger.h"
#include "cppev/nio.h"
#include "cppev/event_loop.h"
#include <fcntl.h>

const int port = 8891;

const char *file = "/tmp/test.file";

int fd;

cppev::fd_event_cb rd_callback = [](std::shared_ptr<cppev::nio> iop) -> void
{
    std::shared_ptr<cppev::nsocktcp> iopt = std::dynamic_pointer_cast<cppev::nsocktcp>(iop);
    int num = iopt->read_all();
    if (num == 0)
    {
        cppev::log::info << "receive file complete" << cppev::log::endl;
        iop->evlp()->fd_remove(iop, true);
        return;
    }

    std::string file_copy = std::string(file) + ".copy";
    fd = open(file_copy.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    if (fd < 0)
    {
        cppev::throw_system_error("open error");
    }
    cppev::nstream ios(fd);

    ios.wbuf()->put(iopt->rbuf()->get());
    ios.write_all();
};

void request_file()
{
    std::shared_ptr<cppev::nsocktcp> iopt = cppev::nio_factory::get_nsocktcp(cppev::family::ipv4);
    iopt->connect("127.0.0.1", port);
    if (!iopt->check_connect())
    {
        cppev::throw_system_error("connect error");
    }
    iopt->wbuf()->put(file);
    iopt->wbuf()->put("\n");
    iopt->write_all();
    
    cppev::event_loop evlp;
    std::shared_ptr<cppev::nio> iop = std::dynamic_pointer_cast<cppev::nio>(iopt);
    evlp.fd_register(iop, cppev::fd_event::fd_readable, rd_callback);
    evlp.loop();
}

int main(int argc, char **argv)
{
    request_file();
    return 0;
}