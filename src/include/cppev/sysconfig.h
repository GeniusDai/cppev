#ifndef _sysconfig_h_6C0224787A17_
#define _sysconfig_h_6C0224787A17_

#define CPPEV_DEBUG

namespace cppev
{

namespace sysconfig
{

// buffer size for reading inotify fd
extern int inotify_step;

// buffer size for udp socket
extern int udp_buffer_size;

// param backlog for ::listen()
extern int listen_number;

// param size for ::epoll_create()
extern int event_number;

// logger thread buffer outdate timespan in seconds
extern int buffer_outdate;

// batch size for IO
extern int buffer_io_step;

}   // namespace sysconfig

}   // namespace cppev

#endif  // sysconfig.h
