#ifndef _cppev_sysconfig_h_6C0224787A17_
#define _cppev_sysconfig_h_6C0224787A17_

// #define CPPEV_DEBUG

namespace cppev
{

namespace sysconfig
{

// Buffer size for udp socket
extern int udp_buffer_size;

// File descriptor numbers for each epoll / kevent
extern int event_number;

// Default batch size for stream's read and write
extern int buffer_io_step;

}   // namespace sysconfig

}   // namespace cppev

#endif  // sysconfig.h
