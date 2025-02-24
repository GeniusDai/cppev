#ifndef _cppev_nio_h_6C0224787A17_
#define _cppev_nio_h_6C0224787A17_

#include <string>
#include <sys/socket.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <exception>
#include <unistd.h>
#include <climits>
#include <iostream>
#include "cppev/utils.h"
#include "cppev/buffer.h"
#include "cppev/sysconfig.h"

namespace cppev
{

class nio;
class nstream;
class nsock;
class nsockudp;
class nsocktcp;
class event_loop;

enum class family
{
    ipv4,
    ipv6,
    local
};

namespace nio_factory
{

std::shared_ptr<nsocktcp> get_nsocktcp(family f);

std::shared_ptr<nsockudp> get_nsockudp(family f);

std::vector<std::shared_ptr<nstream>> get_pipes();

std::vector<std::shared_ptr<nstream>> get_fifos(const std::string &str);

};

class nio
{
public:
    explicit nio(int fd);

    nio(const nio &) = delete;
    nio &operator=(const nio &) = delete;

    nio(nio &&other) noexcept;
    nio &operator=(nio &&other) noexcept;

    virtual ~nio() noexcept;

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

    // Query event loop this nio belongs to
    const event_loop &evlp() const noexcept;

    // Query event loop this nio belongs to
    event_loop &evlp() noexcept;

    // Set event loop this nio belongs to
    void set_evlp(event_loop &evlp) noexcept;

    // Is nio closed
    bool is_closed() const noexcept;

    // Close nio
    void close() noexcept;

    // Set fd to nonblock
    void set_io_nonblock();

    // Set fd to block
    void set_io_block();

protected:
    // File descriptor
    int fd_;

    // Whether closed
    bool closed_;

    // Read buffer
    buffer rbuffer_;

    // Write buffer
    buffer wbuffer_;

    // One nio belongs to one event loop
    event_loop *evlp_;

    // Move constructor implementation
    void move(nio &&other) noexcept;
};

class nstream
: public virtual nio
{
public:
    explicit nstream(int fd);

    nstream(nstream &&other) noexcept;

    nstream &operator=(nstream &&other) noexcept;

    virtual ~nstream() = default;

    // Is connection reset, ECONNRESET
    bool is_reset() const noexcept;

    // End of file
    bool eof() const noexcept;

    // Error of pipe, EPIPE
    bool eop() const noexcept;

    // Read until block or unreadable
    // @param len   Bytes to read, at most len
    // @return      Exact bytes that have been read into rbuffer
    int read_chunk(int len);

    // Write until block or unwritable
    // @param len   Bytes to write, at most len
    // @return      Exact bytes that have been writen from wbuffer
    int write_chunk(int len);

    // Read until block or unreadable
    // @param step  Bytes to read in each loop
    // @return      Exact bytes that have been read into rbuffer
    int read_all(int step = sysconfig::buffer_io_step);

    // Write until block or unwritable
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
    void move(nstream &&other, bool move_base) noexcept;
};

class nsock
: public virtual nio
{
    friend std::shared_ptr<nsocktcp> nio_factory::get_nsocktcp(family f);
    friend std::shared_ptr<nsockudp> nio_factory::get_nsockudp(family f);
public:
    nsock(int fd, family f);

    nsock(nsock &&other) noexcept;

    nsock &operator=(nsock &&other) noexcept;

    virtual ~nsock() = default;

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
    void bind_unix(const std::string &path, bool remove =false);

    // setsockopt SO_REUSEADDR
    void set_so_reuseaddr(bool enable=true);

    // getsockopt SO_REUSEADDR
    bool get_so_reuseaddr() const;

    // setsockopt SO_REUSEPORT
    void set_so_reuseport(bool enable=true);

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

    // setsockopt SO_SNDLOWAT, DONOT use it in linux since protocol not available
    void set_so_sndlowat(int size);

    // getsockopt SO_SNDLOWAT
    int get_so_sndlowat() const;

protected:
    // socket family
    family family_;

    // TCP --> IPv4 / IPv6 :
    //         Record ip/port in connect()
    //         Return by connpeer()
    // TCP --> Unix :
    //         Record sockpath in bind_unix()/connect_unix()
    //         Return by sockname()/peername()/connpeer()
    // UDP --> Unix :
    //         Record sockpath in bind_unix()
    //         Return by recv()
    std::tuple<std::string, int> peer_;

    // Move constructor implementation
    void move(nsock &&other, bool move_base) noexcept;

    static const std::unordered_map<family, int, enum_hash> fmap_;

    static const std::unordered_map<family, int, enum_hash> faddr_len_;
};

enum class shutdown_mode
{
    shutdown_rd,
    shutdown_wr,
    shutdown_rdwr,
};

class nsocktcp final
: public nsock, public nstream
{
public:
    nsocktcp(int sockfd, family f);

    nsocktcp(nsocktcp &&other) noexcept;

    nsocktcp &operator=(nsocktcp &&other) noexcept;

    ~nsocktcp() = default;

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
    std::vector<std::shared_ptr<nsocktcp>> accept(int batch = INT_MAX);

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
    std::tuple<std::string, int, family> connpeer() const noexcept;

    // setsockopt SO_KEEPALIVE
    void set_so_keepalive(bool enable=true);

    // getsockopt SO_KEEPALIVE
    bool get_so_keepalive() const;

    // setsockopt SO_LINGER
    void set_so_linger(bool l_onoff, int l_linger=0);

    // getsockopt SO_LINGER
    std::pair<bool, int> get_so_linger() const;

    // setsockopt TCP_NODELAY
    void set_tcp_nodelay(bool enable=true);

    // getsockopt TCP_NODELAY
    bool get_tcp_nodelay() const;

    // getsockopt SO_ERROR, option cannot be set
    int get_so_error() const;

private:
    // Move constructor implementation
    void move(nsocktcp &&other, bool move_base) noexcept;
};


class nsockudp final
: public nsock
{
public:
    nsockudp(int sockfd, family f);

    nsockudp(nsockudp &&other) noexcept;

    nsockudp &operator=(nsockudp &&other) noexcept;

    ~nsockudp() = default;

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
    void set_so_broadcast(bool enable=true);

    // getsockopt SO_BROADCAST
    bool get_so_broadcast() const;

private:
    // Move constructor implementation
    void move(nsockudp &&other, bool move_base) noexcept;
};

}   // namespace cppev

#endif  // nio.h
