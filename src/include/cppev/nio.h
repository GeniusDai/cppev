#ifndef _nio_h_6C0224787A17_
#define _nio_h_6C0224787A17_

#include <sys/socket.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include <exception>
#include <unistd.h>
#include <climits>
#include <iostream>
#include "cppev/common_utils.h"
#include "cppev/buffer.h"
#include "cppev/sysconfig.h"

namespace cppev
{

class nio;
class nstream;
class nsock;
class nsockudp;
class nsocktcp;
class nwatcher;
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

std::vector<std::shared_ptr<nstream> > get_pipes();

std::vector<std::shared_ptr<nstream> > get_fifos(const std::string &str);

};

class nio
{
public:
    explicit nio(int fd)
    : fd_(fd), closed_(false)
    {
        set_nonblock();
        rbuffer_ = std::unique_ptr<buffer>(new buffer(1));
        wbuffer_ = std::unique_ptr<buffer>(new buffer(1));
    }

    nio(const nio &) = delete;
    nio &operator=(const nio &) = delete;
    nio(nio &&) = delete;
    nio &operator=(nio &&) = delete;

    virtual ~nio() noexcept
    {
        if (!closed_)
        {
             close();
        }
    }

    int fd() const noexcept
    {
        return fd_;
    }

    // Read buffer
    buffer *rbuf() const noexcept
    {
        return rbuffer_.get();
    }

    // Write buffer
    buffer *wbuf() const noexcept
    {
        return wbuffer_.get();
    }

    event_loop *evlp() const noexcept
    {
        return evlp_;
    }

    void set_evlp(event_loop *evlp) noexcept
    {
        evlp_ = evlp;
    }

    bool is_closed() const noexcept
    {
        return closed_;
    }

    void close() noexcept
    {
        ::close(fd_);
        closed_ = true;
    }

protected:
    // File descriptor
    int fd_;

    // Whether closed
    bool closed_;

    // Read buffer, should be initialized
    std::unique_ptr<buffer> rbuffer_;

    // Write buffer, should be initialized
    std::unique_ptr<buffer> wbuffer_;

    // One nio belongs to one event loop
    event_loop *evlp_;

private:
    // Set fd to nonblock
    void set_nonblock() const;
};

class nstream
: public virtual nio
{
public:
    explicit nstream(int fd)
    : nio(fd), reset_(false), eof_(false), eop_(false)
    {}

    virtual ~nstream() = default;

    bool is_reset() const noexcept
    {
        return reset_;
    }

    bool eof() const noexcept
    {
        return eof_;
    }

    bool eop() const noexcept
    {
        return eop_;
    }

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
    int read_all(int step = 128);

    // Write until block or unwritable
    // @param len   Bytes to write in each loop
    // @return      Exact bytes that have been writen from wbuffer
    int write_all(int step = 128);

protected:
    // Used by tcp-socket
    bool reset_;

    // End Of File: Used by tcp-socket, pipe, fifo, disk-file
    bool eof_;

    // Error Of Pipe: Used by tcp-socket, pipe, fifo
    bool eop_;

};

class nsock
: public virtual nio
{
    friend std::shared_ptr<nsocktcp> nio_factory::get_nsocktcp(family f);
    friend std::shared_ptr<nsockudp> nio_factory::get_nsockudp(family f);
public:
    nsock(int fd, family f)
    : nio(fd), family_(f)
    {}

    virtual ~nsock() = default;

    family sockfamily() const noexcept
    {
        return family_;
    }

    // setsockopt SO_REUSEADDR
    void set_so_reuseaddr(bool enable=true) const;

    // getsockopt SO_REUSEADDR
    bool get_so_reuseaddr() const;

    // setsockopt SO_REUSEPORT
    void set_so_reuseport(bool enable=true) const;

    // getsockopt SO_REUSEPORT
    bool get_so_reuseport() const;

    // setsockopt SO_RCVBUF, actual value = size*2 in linux
    void set_so_rcvbuf(int size) const;

    // getsockopt SO_RCVBUF, actual value = size*2 in linux
    int get_so_rcvbuf() const;

    // setsockopt SO_SNDBUF
    void set_so_sndbuf(int size) const;

    // getsockopt SO_SNDBUF
    int get_so_sndbuf() const;

    // setsockopt SO_RCVLOWAT
    void set_so_rcvlowat(int size) const;

    // getsockopt SO_RCVLOWAT
    int get_so_rcvlowat() const;

    // setsockopt SO_SNDLOWAT, Protocol not available in linux
    void set_so_sndlowat(int size) const;

    // getsockopt SO_SNDLOWAT, Protocol not available in linux
    int get_so_sndlowat() const;

protected:
    // socket family
    family family_;

    static const std::unordered_map<family, int, enum_hash> fmap_;

    static const std::unordered_map<family, int, enum_hash> faddr_len_;
};

enum class shut_howto
{
    shut_rd,
    shut_wr,
    shut_rdwr
};

class nsocktcp final
: public nsock, public nstream
{
public:
    nsocktcp(int sockfd, family f)
    : nio(sockfd), nsock(sockfd, f), nstream(sockfd)
    {}

    void listen(int port, const char *ip = nullptr) const;

    void listen(int port, const std::string &ip) const
    {
        listen(port, ip.c_str());
    }

    bool connect(const char *ip, int port);

    bool connect(const std::string &ip, int port)
    {
        return connect(ip.c_str(), port);
    }

    void listen_unix(const char *path, bool remove = false);

    void listen_unix(const std::string &path, bool remove = false)
    {
        listen_unix(path.c_str(), remove);
    }

    bool connect_unix(const char *path);

    bool connect_unix(const std::string &path)
    {
        return connect_unix(path.c_str());
    }

    bool check_connect() const
    {
        return get_so_error() == 0;
    }

    std::vector<std::shared_ptr<nsocktcp> > accept(int batch = INT_MAX) const;

    std::tuple<std::string, int, family> sockname() const;

    std::tuple<std::string, int, family> peername() const;

    std::tuple<std::string, int, family> connpeer() const noexcept
    {
        return std::make_tuple<>(std::get<0>(peer_), std::get<1>(peer_), family_);
    }

    void shutdown(shut_howto howto) const noexcept;

    // setsockopt SO_KEEPALIVE
    void set_so_keepalive(bool enable=true) const;

    // getsockopt SO_KEEPALIVE
    bool get_so_keepalive() const;

    // setsockopt SO_LINGER
    void set_so_linger(bool l_onoff, int l_linger=0) const;

    // getsockopt SO_LINGER
    std::pair<bool, int> get_so_linger() const;

    // setsockopt TCP_NODELAY
    void set_tcp_nodelay(bool enable=true) const;

    // getsockopt TCP_NODELAY
    bool get_tcp_nodelay() const;

    // getsockopt SO_ERROR, option cannot be set
    int get_so_error() const;

private:
    // IPV4/6 : Record ip/port  in connect()
    //          Return by connpeer()
    // Unix   : Record sockpath in listen_unix()/connect_unix()
    //        : Return by sockname()/peername()/connpeer()
    std::tuple<std::string, int> peer_;
};


class nsockudp final
: public nsock
{
public:
    nsockudp(int sockfd, family f) noexcept
    : nio(sockfd), nsock(sockfd, f)
    {}

    void bind(int port, const char *ip = nullptr) const;

    void bind(int port, const std::string &ip) const
    {
        bind(port, ip.c_str());
    }

    void send(const char *ip, int port);

    void send(const std::string &ip, int port)
    {
        send(ip.c_str(), port);
    }

    std::tuple<std::string, int, family> recv();

    void bind_unix(const char *path, bool remove = false);

    void bind_unix(const std::string &path, bool remove = false)
    {
        bind_unix(path.c_str(), remove);
    }

    void send_unix(const char *path);

    void send_unix(const std::string &path)
    {
        send_unix(path.c_str());
    }

    // setsockopt SO_BROADCAST
    void set_so_broadcast(bool enable=true) const;

    // getsockopt SO_BROADCAST
    bool get_so_broadcast() const;

private:
    std::string unix_listen_path_;
};

}   // namespace cppev

#endif  // nio.h