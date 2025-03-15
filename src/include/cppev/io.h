#ifndef _cppev_io_h_6C0224787A17_
#define _cppev_io_h_6C0224787A17_

#include <sys/socket.h>
#include <unistd.h>

#include <climits>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "cppev/buffer.h"
#include "cppev/common.h"
#include "cppev/utils.h"

namespace cppev
{

class io;
class stream;
class sock;
class sockudp;
class socktcp;
class event_loop;

enum class CPPEV_PUBLIC family
{
    ipv4,
    ipv6,
    local,
};

namespace io_factory
{

CPPEV_PUBLIC std::shared_ptr<socktcp> get_socktcp(family f);

CPPEV_PUBLIC std::shared_ptr<sockudp> get_sockudp(family f);

CPPEV_PUBLIC std::vector<std::shared_ptr<stream>> get_pipes();

CPPEV_PUBLIC std::vector<std::shared_ptr<stream>> get_fifos(
    const std::string &str);

};  // namespace io_factory

class CPPEV_PUBLIC io
{
public:
    explicit io(int fd, bool block = false);

    io(const io &) = delete;
    io &operator=(const io &) = delete;

    io(io &&other) noexcept;
    io &operator=(io &&other) noexcept;

    virtual ~io() noexcept;

    // File descriptor.
    int fd() const noexcept;

    // Read buffer.
    const buffer &rbuffer() const noexcept;

    // Read buffer.
    buffer &rbuffer() noexcept;

    // Write buffer.
    const buffer &wbuffer() const noexcept;

    // Write buffer.
    buffer &wbuffer() noexcept;

    // Query event loop this io belongs to.
    const event_loop &evlp() const noexcept;

    // Query event loop this io belongs to.
    event_loop &evlp() noexcept;

    // Set event loop this io belongs to.
    // @param evlp  Pointer of event loop.
    void set_evlp(event_loop *evlp) noexcept;

    // Is io closed.
    bool is_closed() const noexcept;

    // Close io.
    void close() noexcept;

    // Set fd to nonblock.
    void set_io_nonblock();

    // Set fd to block.
    void set_io_block();

protected:
    // File descriptor.
    int fd_;

    // Whether block io.
    bool block_;

    // Whether closed.
    bool closed_;

    // Read buffer.
    buffer rbuffer_;

    // Write buffer.
    buffer wbuffer_;

    // One io belongs to one event loop.
    event_loop *evlp_;

    // Move constructor implementation.
    void move(io &&other) noexcept;
};

class CPPEV_PUBLIC stream : public virtual io
{
public:
    explicit stream(int fd);

    stream(stream &&other) noexcept;

    stream &operator=(stream &&other) noexcept;

    virtual ~stream();

    // Is connection reset, ECONNRESET.
    bool is_reset() const noexcept;

    // End of file.
    bool eof() const noexcept;

    // Error of pipe, EPIPE.
    bool eop() const noexcept;

    // Block IO:    Read until finishing len bytes or block.
    // Nonblock IO: Read until finishing len bytes or kernel receving buffer
    //              empty.
    // @param len   Bytes to read in rbuffer, at most len.
    // @return      Exact bytes that have been read into rbuffer.
    int read_chunk(int len);

    // Block IO:    Write until finishing len bytes or block.
    // Nonblock IO: Write until finishing len bytes or kernel sending buffer
    //              full.
    // @param len   Bytes to write in wbuffer, at most len.
    // @return      Exact bytes that have been writen from wbuffer.
    int write_chunk(int len);

    // Block IO:    Throw std::logic_error.
    // Nonblock IO: Read until kernel receving buffer is empty.
    // @param step  Bytes to read in each loop.
    // @return      Exact bytes that have been read into rbuffer.
    int read_all(int step = sysconfig::buffer_io_step);

    // Block IO:    Throw std::logic_error.
    // Nonblock IO: Write until kernel sending buffer is full or user
    //              wbuffer is empty.
    // @param step  Bytes to write in each loop.
    // @return      Exact bytes that have been writen from wbuffer.
    int write_all(int step = sysconfig::buffer_io_step);

protected:
    // Connect Reset: Used by tcp-socket.
    bool reset_;

    // End Of File: Used by tcp-socket, pipe, fifo, disk-file.
    bool eof_;

    // Error Of Pipe: Used by tcp-socket, pipe, fifo.
    bool eop_;

    // Move constructor implementation.
    void move(stream &&other, bool move_base) noexcept;
};

class CPPEV_PUBLIC sock : public virtual io
{
    friend std::shared_ptr<socktcp> io_factory::get_socktcp(family f);
    friend std::shared_ptr<sockudp> io_factory::get_sockudp(family f);

public:
    sock(int fd, family f);

    sock(sock &&other) noexcept;

    sock &operator=(sock &&other) noexcept;

    virtual ~sock();

    // Socket family.
    family sockfamily() const noexcept;

    // Syscall bind to address for ipv4 / ipv6.
    // @param ip    IP to bind.
    // @param port  Port to bind.
    void bind(const char *ip, int port);

    // Syscall bind to address for ipv4 / ipv6.
    // @param port  Port to bind.
    void bind(int port);

    // Syscall bind to address for ipv4 / ipv6.
    // @param ip    IP to bind.
    // @param port  Port to bind.
    void bind(const std::string &ip, int port);

    // Syscall bind to address for unix-domain.
    // @param path      Socket file's path to bind.
    // @param remove    Remove socket file if already exists.
    void bind_unix(const char *path, bool remove = false);

    // Syscall bind to address for unix-domain.
    // @param path      Socket file's path to bind.
    // @param remove    Remove socket file if already exists.
    void bind_unix(const std::string &path, bool remove = false);

    // Syscall setsockopt SO_REUSEADDR.
    // Set whether reuse address even in TIME_WAIT status.
    // @param enable    Enable reuse address or not.
    void set_so_reuseaddr(bool enable = true);

    // Syscall getsockopt SO_REUSEADDR.
    // Get whether reuse address even in TIME_WAIT status.
    bool get_so_reuseaddr() const;

    // Syscall setsockopt SO_REUSEPORT.
    // Set whether allow multiple processes or threads bind to same port.
    // @param enable    Enable reuse port or not.
    void set_so_reuseport(bool enable = true);

    // Syscall getsockopt SO_REUSEPORT.
    // Get whether allow multiple processes or threads bind to same port.
    bool get_so_reuseport() const;

    // Syscall setsockopt SO_RCVBUF.
    // Set socket's receiving buffer, actually set to size*2 in Linux.
    // @param size  Buffer size.
    void set_so_rcvbuf(int size);

    // Syscall getsockopt SO_RCVBUF.
    // Get socket's receiving buffer.
    int get_so_rcvbuf() const;

    // Syscall setsockopt SO_SNDBUF.
    // Set socket's sending buffer, actually set to size*2 in Linux.
    // @param size  Buffer size.
    void set_so_sndbuf(int size);

    // Syscall getsockopt SO_SNDBUF.
    // Get socket's sending buffer.
    int get_so_sndbuf() const;

    // Syscall setsockopt SO_RCVLOWAT.
    // Set low water mark to trigger readable event for io multiplexing.
    // @param size  Low water mark to set.
    void set_so_rcvlowat(int size);

    // Syscall getsockopt SO_RCVLOWAT.
    // Get low water mark to trigger readable event for io multiplexing.
    int get_so_rcvlowat() const;

    // Syscall setsockopt SO_SNDLOWAT.
    // Set low water mark to trigger writable event for io multiplexing.
    // Protocol not available in Linux.
    // @param size  Low water mark to set.
    void set_so_sndlowat(int size);

    // Syscall getsockopt SO_SNDLOWAT.
    // Get low water mark to trigger writable event for io multiplexing.
    int get_so_sndlowat() const;

protected:
    // socket family.
    family family_;

    // Record path for bind_unix.
    std::string unix_path_;

    // Move constructor implementation.
    void move(sock &&other, bool move_base) noexcept;
};

enum class CPPEV_PUBLIC shutdown_mode
{
    shutdown_rd,
    shutdown_wr,
    shutdown_rdwr,
};

class CPPEV_PUBLIC socktcp final : public sock, public stream
{
public:
    socktcp(int sockfd, family f);

    socktcp(socktcp &&other) noexcept;

    socktcp &operator=(socktcp &&other) noexcept;

    ~socktcp();

    // Syscall listen for ipv4 / ipv6 / unix-domain.
    // @param backlog   Maximum backlog size.
    void listen(int backlog = SOMAXCONN);

    // Syscall listen for ipv4 / ipv6.
    // @param ip    Target ip to connect.
    // @param port  Target port to connect.
    bool connect(const char *ip, int port);

    // Syscall listen for ipv4 / ipv6.
    // @param ip    Target ip to connect.
    // @param port  Target port to connect.
    bool connect(const std::string &ip, int port);

    // Syscall listen for unix-domain.
    // @param path  Socket file's path to connect.
    bool connect_unix(const char *path);

    // Syscall listen for unix-domain.
    // @param path  Socket file's path to connect.
    bool connect_unix(const std::string &path);

    // Syscall accept for ipv4 / ipv6 / unix-domain.
    // Block IO:    Always accept the exactly number of connected sockets, may
    //              get block.
    // Nonblock IO: Try to accept the exactly number of connected sockets until
    //              no connected sockets are in the backlog.
    // @param batch     Batch size of connected socket to accept.
    std::vector<std::shared_ptr<socktcp>> accept(int batch = INT_MAX);

    // Syscall shutdown for ipv4 / ipv6 / unix-domain.
    // @param howto     shutdown read or write or both.
    void shutdown(shutdown_mode howto) noexcept;

    // Check whether connection is established, used by tcp client.
    bool check_connect() const;

    // Current socket listening uri.
    // Unix-domain: path, -1, family.
    // IPv4 / IPv6: ip, port, family.
    std::tuple<std::string, int, family> sockname() const;

    // Connect target established.
    // Unix-domain: path, -1, family.
    // IPv4 / IPv6: ip, port, family.
    std::tuple<std::string, int, family> peername() const;

    // Connect target even not established.
    // Unix-domain: path, -1, family.
    // IPv4 / IPv6: ip, port, family.
    std::tuple<std::string, int, family> target_uri() const noexcept;

    // Syscall setsockopt SO_KEEPALIVE.
    // Set whether to periodically send ACK to confirm peer's status.
    // @param enable    Enable or not.
    void set_so_keepalive(bool enable = true);

    // Syscall getsockopt SO_KEEPALIVE.
    // Get whether to periodically send ACK to confirm peer's status.
    bool get_so_keepalive() const;

    // Syscall setsockopt SO_LINGER.
    // Set whether to close immediately when close is called.
    // @param l_onoff   0 means closing immediately
    //                  1 means decided by l_linger's value.
    // @param l_linger  0 means sending RST, no FIN / ACK.
    //                  >0 means blocking until data sending finish.
    void set_so_linger(bool l_onoff, int l_linger = 0);

    // Syscall getsockopt SO_LINGER.
    // Get whether to close immediately when close is called.
    std::pair<bool, int> get_so_linger() const;

    // Syscall setsockopt TCP_NODELAY.
    // Set whether Nagle's algorithm should be disabled, the Nagle's algorithm
    // allows small data sending immediately without being merged.
    // @param disable   Disable or not.
    void set_tcp_nodelay(bool disable = true);

    // Syscall getsockopt TCP_NODELAY.
    // Get whether Nagle's algorithm should be disabled, the Nagle's algorithm
    // allows small data sending immediately without being merged.
    bool get_tcp_nodelay() const;

    // Syscall getsockopt SO_ERROR.
    // Used for tcp client connection establish checking which cannot be set.
    int get_so_error() const;

private:
    // Record uri for connect and connect_unix.
    std::tuple<std::string, int> conn_uri_;

    // Move constructor implementation.
    void move(socktcp &&other, bool move_base) noexcept;
};

class CPPEV_PUBLIC sockudp final : public sock
{
public:
    sockudp(int sockfd, family f);

    sockudp(sockudp &&other) noexcept;

    sockudp &operator=(sockudp &&other) noexcept;

    ~sockudp();

    // Syscall recvfrom for ipv4 / ipv6 / unix-domain.
    // Should always check rbuffer for message receving from peer,
    // message may be incomplete if user rbuffer size is too small.
    // For block io will block if kernel receiving buffer is empty.
    // @return  Peer's uri.
    std::tuple<std::string, int, family> recv();

    // Syscall sendto for ipv4 / ipv6.
    // Should always check wbuffer for message remains unsending,
    // may send incomplete message if kernel sending buffer's
    // remaining size is too small. For block io will block if kernel
    // sending buffer is full.
    // @param ip    Target ip to send.
    // @param port  Target port to send.
    void send(const char *ip, int port);

    // Syscall sendto for ipv4 / ipv6.
    // Should always check wbuffer for message remains unsending,
    // may send incomplete message if kernel sending buffer's
    // remaining size is too small. For block io will block if kernel
    // sending buffer is full.
    // @param ip    Target ip to send.
    // @param port  Target port to send.
    void send(const std::string &ip, int port);

    // Syscall sendto for unix-domain.
    // Should always check wbuffer for message remains unsending,
    // may send incomplete message if kernel sending buffer's
    // remaining size is too small. For block io will block if kernel
    // sending buffer is full.
    // @param path  Target socket file's path.
    void send_unix(const char *path);

    // Syscall sendto for unix-domain.
    // Should always check wbuffer for message remains unsending,
    // may send incomplete message if kernel sending buffer's
    // remaining size is too small. For block io will block if kernel
    // sending buffer is full.
    // @param path  Target socket file's path.
    void send_unix(const std::string &path);

    // Syscall setsockopt SO_BROADCAST.
    // Set whether allow udp socket broadcast or not.
    // @enable  Enable broadcast ot not.
    void set_so_broadcast(bool enable = true);

    // Syscall getsockopt SO_BROADCAST.
    bool get_so_broadcast() const;

private:
    // Move constructor implementation.
    void move(sockudp &&other, bool move_base) noexcept;
};

}  // namespace cppev

#endif  // io.h
