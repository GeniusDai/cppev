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

    // File descriptor
    int fd() const noexcept;

    // Read buffer
    const buffer &rbuffer() const noexcept;

    // Read buffer
    buffer &rbuffer() noexcept;

    // Write buffer
    const buffer &wbuffer() const noexcept;

    // Write buffer
    buffer &wbuffer() noexcept;

    // Query event loop this io belongs to
    const event_loop &evlp() const noexcept;

    // Query event loop this io belongs to
    event_loop &evlp() noexcept;

    // Set event loop this io belongs to
    void set_evlp(event_loop *evlp) noexcept;

    // Is io closed
    bool is_closed() const noexcept;

    // Close io
    void close() noexcept;

    // Set fd to nonblock
    void set_io_nonblock();

    // Set fd to block
    void set_io_block();

protected:
    // File descriptor
    int fd_;

    // Whether block io
    bool block_;

    // Whether closed
    bool closed_;

    // Read buffer
    buffer rbuffer_;

    // Write buffer
    buffer wbuffer_;

    // One io belongs to one event loop
    event_loop *evlp_;

    // Move constructor implementation
    void move(io &&other) noexcept;
};

class CPPEV_PUBLIC stream : public virtual io
{
public:
    explicit stream(int fd);

    stream(stream &&other) noexcept;

    stream &operator=(stream &&other) noexcept;

    virtual ~stream();

    // Is connection reset, ECONNRESET
    bool is_reset() const noexcept;

    // End of file
    bool eof() const noexcept;

    // Error of pipe, EPIPE
    bool eop() const noexcept;

    // Block IO:    Read until finishing len bytes or block
    // Nonblock IO: Read until finishing len bytes or io kernel buffer empty
    // @param len   Bytes to read, at most len
    // @return      Exact bytes that have been read into rbuffer
    int read_chunk(int len);

    // Block IO:    Write until finishing len bytes or block
    // Nonblock IO: Write until finishing len bytes or io kernel buffer full
    // @param len   Bytes to write, at most len
    // @return      Exact bytes that have been writen from wbuffer
    int write_chunk(int len);

    // Block IO:    Throw std::logic_error
    // Nonblock IO: Read until io kernel buffer empty
    // @param step  Bytes to read in each loop
    // @return      Exact bytes that have been read into rbuffer
    int read_all(int step = sysconfig::buffer_io_step);

    // Block IO:    Throw std::logic_error
    // Nonblock IO: Write until io kernel buffer full or user buffer empty
    // @param step  Bytes to write in each loop
    // @return      Exact bytes that have been writen from wbuffer
    int write_all(int step = sysconfig::buffer_io_step);

protected:
    // Connect Reset: Used by tcp-socket
    bool reset_;

    // End Of File: Used by tcp-socket, pipe, fifo, disk-file
    bool eof_;

    // Error Of Pipe: Used by tcp-socket, pipe, fifo
    bool eop_;

    // Move constructor implementation
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

    // socket family
    family sockfamily() const noexcept;

    // bind to address: IPv4 / IPv6
    void bind(const char *ip, int port);

    // bind to address: IPv4 / IPv6
    void bind(int port);

    // bind to address: IPv4 / IPv6
    void bind(const std::string &ip, int port);

    // bind to address: Unix-domain
    void bind_unix(const char *path, bool remove = false);

    // bind to address: Unix-domain
    void bind_unix(const std::string &path, bool remove = false);

    // setsockopt SO_REUSEADDR
    void set_so_reuseaddr(bool enable = true);

    // getsockopt SO_REUSEADDR
    bool get_so_reuseaddr() const;

    // setsockopt SO_REUSEPORT
    void set_so_reuseport(bool enable = true);

    // getsockopt SO_REUSEPORT
    bool get_so_reuseport() const;

    // setsockopt SO_RCVBUF, actual value = size*2 in linux
    void set_so_rcvbuf(int size);

    // getsockopt SO_RCVBUF
    int get_so_rcvbuf() const;

    // setsockopt SO_SNDBUF, actual value = size*2 in linux
    void set_so_sndbuf(int size);

    // getsockopt SO_SNDBUF
    int get_so_sndbuf() const;

    // setsockopt SO_RCVLOWAT
    void set_so_rcvlowat(int size);

    // getsockopt SO_RCVLOWAT
    int get_so_rcvlowat() const;

    // setsockopt SO_SNDLOWAT, DONOT use it in linux since protocol not
    // available
    void set_so_sndlowat(int size);

    // getsockopt SO_SNDLOWAT
    int get_so_sndlowat() const;

protected:
    // socket family
    family family_;

    // Record path for bind_unix
    std::string unix_path_;

    // Move constructor implementation
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

    // listen: IPv4 / IPv6 / Unix-domain
    void listen(int backlog = SOMAXCONN);

    // connect: IPv4 / IPv6
    bool connect(const char *ip, int port);

    // connect: IPv4 / IPv6
    bool connect(const std::string &ip, int port);

    // connect: Unix-domain
    bool connect_unix(const char *path);

    // connect: Unix-domain
    bool connect_unix(const std::string &path);

    // accept: IPv4 / IPv6 / Unix-domain
    // Block IO:    Always accept the exactly number of connected sockets, may
    //              get block.
    // Nonblock IO: Try to accept the exactly number of connected sockets until
    //              no connected sockets are in the backlog.
    std::vector<std::shared_ptr<socktcp>> accept(int batch = INT_MAX);

    // shutdown: IPv4 / IPv6 / Unix-domain
    void shutdown(shutdown_mode howto) noexcept;

    // whether connect is established, used by tcp client
    bool check_connect() const;

    // Current socket: ip / port / family
    // For Unix-domain: ip=path, port=-1
    std::tuple<std::string, int, family> sockname() const;

    // Connect target established: ip / port / family
    // For Unix-domain: ip=path, port=-1
    std::tuple<std::string, int, family> peername() const;

    // Connect target even not established: ip / port / family
    // For Unix-domain: ip=path, port=-1
    std::tuple<std::string, int, family> target_uri() const noexcept;

    // setsockopt SO_KEEPALIVE
    void set_so_keepalive(bool enable = true);

    // getsockopt SO_KEEPALIVE
    bool get_so_keepalive() const;

    // setsockopt SO_LINGER
    void set_so_linger(bool l_onoff, int l_linger = 0);

    // getsockopt SO_LINGER
    std::pair<bool, int> get_so_linger() const;

    // setsockopt TCP_NODELAY
    void set_tcp_nodelay(bool enable = true);

    // getsockopt TCP_NODELAY
    bool get_tcp_nodelay() const;

    // getsockopt SO_ERROR, option cannot be set
    int get_so_error() const;

private:
    // Record uri for connect and connect_unix
    std::tuple<std::string, int> conn_uri_;

    // Move constructor implementation
    void move(socktcp &&other, bool move_base) noexcept;
};

class CPPEV_PUBLIC sockudp final : public sock
{
public:
    sockudp(int sockfd, family f);

    sockudp(sockudp &&other) noexcept;

    sockudp &operator=(sockudp &&other) noexcept;

    ~sockudp();

    // recvfrom: IPv4 / IPv6 / Unix-domain
    std::tuple<std::string, int, family> recv();

    // sendto: IPv4 / IPv6
    void send(const char *ip, int port);

    // sendto: IPv4 / IPv6
    void send(const std::string &ip, int port);

    // sendto: Unix-domain
    void send_unix(const char *path);

    // sendto: Unix-domain
    void send_unix(const std::string &path);

    // setsockopt SO_BROADCAST
    void set_so_broadcast(bool enable = true);

    // getsockopt SO_BROADCAST
    bool get_so_broadcast() const;

private:
    // Move constructor implementation
    void move(sockudp &&other, bool move_base) noexcept;
};

}  // namespace cppev

#endif  // io.h
