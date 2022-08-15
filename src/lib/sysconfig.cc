#include "cppev/sysconfig.h"
#include <sys/socket.h>

namespace cppev
{

namespace sysconfig
{

// buffer size for reading linux inotify fd
int inotify_step = 1024;

// buffer size for udp socket
int udp_buffer_size = 2048;

// param backlog for listen()
int listen_number = SOMAXCONN;

// param size for epoll_create()/epoll_wait()/kevent()
int event_number = 2048;

// hashed logger thread buffer outdate timespan in seconds
int buffer_outdate = 30;

// batch size for IO
int buffer_io_step = 1024;

}   // namespace sysconfig

}   // namespace cppev
