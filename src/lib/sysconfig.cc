#include "cppev/sysconfig.h"

namespace cppev
{

namespace sysconfig
{

// buffer size for reading inotify fd
int inotify_step = 1024;

// buffer size for udp socket
int udp_buffer_size = 2048;

// param backlog for ::listen()
int listen_number = 1024;

// param size for ::epoll_create() and ::epoll_wait()
int event_number = 2048;

// logger thread buffer outdate timespan in seconds
int buffer_outdate = 30;

// batch size for IO
int buffer_io_step = 1024;

}   // namespace sysconfig

}   // namespace cppev
