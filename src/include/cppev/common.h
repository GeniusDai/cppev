#ifndef _cppev_common_h_6C0224787A17_
#define _cppev_common_h_6C0224787A17_

#define CPPEV_PUBLIC __attribute__((visibility("default")))
#define CPPEV_INTERNAL __attribute__((visibility("default")))
#define CPPEV_PRIVATE __attribute__((visibility("hidden")))

namespace cppev
{

namespace sysconfig
{

// Buffer size for udp socket
CPPEV_PUBLIC extern int udp_buffer_size;

// File descriptor numbers for each epoll / kevent
CPPEV_PUBLIC extern int event_number;

// Default batch size for stream's read and write
CPPEV_PUBLIC extern int buffer_io_step;

}  // namespace sysconfig

}  // namespace cppev

#endif  // common.h
