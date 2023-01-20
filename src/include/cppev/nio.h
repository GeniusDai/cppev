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
    }

    nio(const nio &) = delete;
    nio &operator=(const nio &) = delete;

    nio(nio &&other) noexcept
    {
        move(std::forward<nio>(other));
    }

    nio &operator=(nio &&other) noexcept
    {
        move(std::forward<nio>(other));
        return *this;
    }

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
    const buffer &rbuffer() const noexcept
    {
        return rbuffer_;
    }

    buffer &rbuffer() noexcept
    {
        return rbuffer_;
    }

    // Write buffer
    const buffer &wbuffer() const noexcept
    {
        return wbuffer_;
    }

    buffer &wbuffer() noexcept
    {
        return wbuffer_;
    }

    const event_loop &evlp() const noexcept
    {
        return *evlp_;
    }

    event_loop &evlp() noexcept
    {
        return *evlp_;
    }

    void set_evlp(event_loop &evlp) noexcept
    {
        evlp_ = &evlp;
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

    // Read buffer
    buffer rbuffer_;

    // Write buffer
    buffer wbuffer_;

    // One nio belongs to one event loop
    event_loop *evlp_;

    void move(nio &&other) noexcept
    {
        this->fd_ = other.fd_;
        this->closed_ = other.closed_;
        this->rbuffer_ = std::move(other.rbuffer_);
        this->wbuffer_ = std::move(other.rbuffer_);
        this->evlp_ = other.evlp_;

        other.fd_ = -1;
        other.closed_ = true;
        other.evlp_ = nullptr;
    }

private:
    // Set fd to nonblock
    void set_nonblock();
};

class nstream
: public virtual nio
{
public:
    explicit nstream(int fd)
    : nio(fd), reset_(false), eof_(false), eop_(false)
    {
    }

    nstream(nstream &&other) noexcept
    : nio(std::forward<nstream>(other))
    {
        move(std::forward<nstream>(other), false);
    }

    nstream &operator=(nstream &&other) noexcept
    {
        move(std::forward<nstream>(other), true);
        return *this;
    }

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

    void move(nstream &&other, bool move_base) noexcept
    {
        if (move_base)
        {
            nio::move(std::forward<nstream>(other));
        }
        this->reset_ = other.reset_;
        this->eof_ = other.eof_;
        this->eop_ = other.eop_;
    }
};

class nsock
: public virtual nio
{
    friend std::shared_ptr<nsocktcp> nio_factory::get_nsocktcp(family f);
    friend std::shared_ptr<nsockudp> nio_factory::get_nsockudp(family f);
public:
    nsock(int fd, family f)
    : nio(fd), family_(f)
    {
    }

    nsock(nsock &&other) noexcept
    : nio(std::forward<nsock>(other))
    {
        move(std::forward<nsock>(other), false);
    }

    nsock &operator=(nsock &&other) noexcept
    {
        move(std::forward<nsock>(other), true);
        return *this;
    }

    virtual ~nsock() = default;

    family sockfamily() const noexcept
    {
        return family_;
    }

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

    // getsockopt SO_RCVBUF, actual value = size*2 in linux
    int get_so_rcvbuf() const;

    // setsockopt SO_SNDBUF
    void set_so_sndbuf(int size);

    // getsockopt SO_SNDBUF
    int get_so_sndbuf() const;

    // setsockopt SO_RCVLOWAT
    void set_so_rcvlowat(int size);

    // getsockopt SO_RCVLOWAT
    int get_so_rcvlowat() const;

    // setsockopt SO_SNDLOWAT, Protocol not available in linux
    void set_so_sndlowat(int size);

    // getsockopt SO_SNDLOWAT, Protocol not available in linux
    int get_so_sndlowat() const;

protected:
    // socket family
    family family_;

    static const std::unordered_map<family, int, enum_hash> fmap_;

    static const std::unordered_map<family, int, enum_hash> faddr_len_;

    void move(nsock &&other, bool move_base) noexcept
    {
        if (move_base)
        {
            nio::move(std::forward<nsock>(other));
        }
        this->family_ = other.family_;
    }
};

enum class shut_mode
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
    {
    }

    nsocktcp(nsocktcp &&other) noexcept
    : nio(std::forward<nsocktcp>(other)),
      nsock(std::forward<nsocktcp>(other)),
      nstream(std::forward<nsocktcp>(other))
    {
        move(std::forward<nsocktcp>(other), false);
    }

    nsocktcp &operator=(nsocktcp &&other) noexcept
    {
        move(std::forward<nsocktcp>(other), true);
        return *this;
    }

    ~nsocktcp() = default;

    void listen(int port, const char *ip = nullptr);

    void listen(int port, const std::string &ip)
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

    std::vector<std::shared_ptr<nsocktcp> > accept(int batch = INT_MAX);

    std::tuple<std::string, int, family> sockname() const;

    std::tuple<std::string, int, family> peername() const;

    std::tuple<std::string, int, family> connpeer() const noexcept
    {
        return std::make_tuple<>(std::get<0>(peer_), std::get<1>(peer_), family_);
    }

    void shutdown(shut_mode howto) noexcept;

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
    // IPV4/6 : Record ip/port  in connect()
    //          Return by connpeer()
    // Unix   : Record sockpath in listen_unix()/connect_unix()
    //        : Return by sockname()/peername()/connpeer()
    std::tuple<std::string, int> peer_;

    void move(nsocktcp &&other, bool move_base) noexcept
    {
        if (move_base)
        {
            nio::move(std::forward<nsocktcp>(other));
            nsock::move(std::forward<nsocktcp>(other), false);
            nstream::move(std::forward<nsocktcp>(other), false);
        }
        this->peer_ = other.peer_;
    }
};


class nsockudp final
: public nsock
{
public:
    nsockudp(int sockfd, family f) noexcept
    : nio(sockfd), nsock(sockfd, f)
    {
    }

    nsockudp(nsockudp &&other) noexcept
    : nio(std::forward<nsockudp>(other)), nsock(std::forward<nsockudp>(other))
    {
        move(std::forward<nsockudp>(other), false);
    }

    nsockudp &operator=(nsockudp &&other) noexcept
    {
        move(std::forward<nsockudp>(other), true);
        return *this;
    }

    ~nsockudp() = default;

    void bind(int port, const char *ip = nullptr);

    void bind(int port, const std::string &ip)
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
    void set_so_broadcast(bool enable=true);

    // getsockopt SO_BROADCAST
    bool get_so_broadcast() const;

private:
    std::string unix_listen_path_;

    void move(nsockudp &&other, bool move_base) noexcept
    {
        if (move_base)
        {
            nsock::move(std::forward<nsockudp>(other), true);
        }

        this->unix_listen_path_ = other.unix_listen_path_;
        other.unix_listen_path_ = "";
    }
};

}   // namespace cppev

#endif  // nio.h