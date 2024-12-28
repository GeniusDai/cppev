#include "cppev/sysconfig.h"

namespace cppev
{

namespace sysconfig
{

// Buffer size for udp socket
int udp_buffer_size = 2048;

// File descriptor numbers for each epoll / kevent
int event_number = 2048;

// Default batch size for stream's read and write
int buffer_io_step = 1024;

}   // namespace sysconfig

}   // namespace cppev
