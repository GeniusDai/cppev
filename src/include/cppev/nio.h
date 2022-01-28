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
#include "cppev/async_logger.h"

#ifdef __linux__
#include <sys/inotify.h>
#endif

namespace cppev {

class nio;
class nstream;
class nsock;
class nsockudp;
class nsocktcp;
class nwatcher;
class event_loop;

enum class family {
    ipv4,
    ipv6,
    local
};

class nio : public uncopyable {
public:
    explicit nio(int fd) : fd_(fd)
    {
        set_nonblock();
        rbuffer_ = std::unique_ptr<buffer>(new buffer(1));
        wbuffer_ = std::unique_ptr<buffer>(new buffer(1));
    }

    virtual ~nio() { close(fd_); }

    int fd() const { return fd_; }

    // read buffer
    buffer *rbuf() { return rbuffer_.get(); }

    // write buffer
    buffer *wbuf() { return wbuffer_.get(); }

    event_loop *evlp() { return evlp_; }

    void set_evlp(event_loop *evlp) { evlp_ = evlp; }

protected:
    // File descriptor
    int fd_;

    // read buffer, should be initialized
    std::unique_ptr<buffer> rbuffer_;

    // write buffer, should be initialized
    std::unique_ptr<buffer> wbuffer_;

    event_loop *evlp_;

private:
    // set fd to nonblock
    void set_nonblock();
};


class nstream : public virtual nio {
public:
    explicit nstream(int fd)
    : nio(fd), reset_(false), eof_(false), eop_(false) {}

    virtual ~nstream() {}

    bool is_reset() { return reset_; }

    bool eof() { return eof_; }

    bool eop() { return eop_; }

    // read until block or unreadable, at most len
    int read_chunk(int len);

    // write until block or unwritable, at most len
    int write_chunk(int len);

    // read until block or unreadable
    int read_all(int step = 128);

    // write until block or unwritable
    int write_all(int step = 128);

protected:
    // Used by tcp-socket
    bool reset_;

    // End Of File: Used by tcp-socket, pipe, fifo, disk-file
    bool eof_;

    // Error Of Pipe: Used by tcp-socket, pipe, fifo
    bool eop_;

};


class nsock : public virtual nio {
    friend class nio_factory;
public:
    nsock(int fd, family f) : nio(fd), family_(f) {}

    virtual ~nsock() {}

    family sockfamily() { return family_; }

    // setsockopt SO_REUSEADDR
    void set_so_reuseaddr(bool enable=true);

    // getsockopt SO_REUSEADDR
    bool get_so_reuseaddr();

    // setsockopt SO_REUSEPORT
    void set_so_reuseport(bool enable=true);

    // getsockopt SO_REUSEPORT
    bool get_so_reuseport();

    // setsockopt SO_RCVBUF
    void set_so_rcvbuf(int size);

    // getsockopt SO_RCVBUF
    int get_so_rcvbuf();

    // setsockopt SO_SNDBUF
    void set_so_sndbuf(int size);

    // getsockopt SO_SNDBUF
    int get_so_sndbuf();

    // setsockopt SO_RCVLOWAT
    void set_so_rcvlowat(int size);

    // getsockopt SO_RCVLOWAT
    int get_so_rcvlowat();

    // setsockopt SO_SNDLOWAT
    void set_so_sndlowat(int size);

    // getsockopt SO_SNDLOWAT
    int get_so_sndlowat();

protected:
    // socket family
    family family_;

    static const std::unordered_map<family, int, enum_hash> fmap_;

    static const std::unordered_map<family, int, enum_hash> faddr_len_;
};

enum class shut_howto {
    shut_rd,
    shut_wr,
    shut_rdwr
};

class nsocktcp final : public nsock, public nstream  {
public:
    nsocktcp(int sockfd, family f)
    : nio(sockfd), nsock(sockfd, f), nstream(sockfd) {}

    void listen(const int port, const char *ip = nullptr);

    void listen(const int port, const std::string &ip) { listen(port, ip.c_str()); }

    void connect(const char *ip, const int port);

    void connect(const std::string &ip, const int port) { connect(ip.c_str(), port); }

    void listen_unix(const char *path);

    void listen_unix(const std::string &path) { listen_unix(path.c_str()); }

    void connect_unix(const char *path);

    void connect_unix(const std::string &path) { connect_unix(path.c_str()); }

    bool check_connect();

    std::vector<std::shared_ptr<nsocktcp> > accept(int batch = INT_MAX);

    std::tuple<std::string, int, family> sockname();

    std::tuple<std::string, int, family> peername();

    std::tuple<std::string, int, family> connpeer() {
        return std::make_tuple<>(peer_.first, peer_.second, family_);
    }

    void shutdown(shut_howto howto);

    // setsockopt SO_KEEPALIVE
    void set_so_keepalive(bool enable=true);

    // getsockopt SO_KEEPALIVE
    bool get_so_keepalive();

    // setsockopt SO_LINGER
    void set_so_linger(bool l_onoff, int l_linger);

    // getsockopt SO_LINGER
    std::pair<bool, int> get_so_linger();

    // setsockopt TCP_NODELAY
    void set_tcp_nodelay(bool enable=true);

    // getsockopt TCP_NODELAY
    bool get_tcp_nodelay();

private:
    // Record peer for connect syscall
    std::pair<std::string, int> peer_;
};


class nsockudp final : public nsock {
public:
    nsockudp(int sockfd, family f)
    : nio(sockfd), nsock(sockfd, f) {}

    void bind(const int port, const char *ip = nullptr);

    void bind(const int port, const std::string &ip) { bind(port, ip.c_str()); }

    void send(const char *ip, const int port);

    void send(const std::string &ip, const int port) { send(ip.c_str(), port); }

    std::tuple<std::string, int, family> recv();

    void bind_unix(const char *path);

    void bind_unix(const std::string &path) { bind_unix(path.c_str()); }

    void send_unix(const char *path);

    void send_unix(const std::string &path) { send_unix(path.c_str()); }

    void recv_unix();

    // setsockopt SO_BROADCAST
    void set_so_broadcast(bool enable=true);

    // getsockopt SO_BROADCAST
    bool get_so_broadcast();
};

#ifdef __linux__

typedef void(*fs_handler)(inotify_event *, const char *path);

class nwatcher final : public nstream {
public:
    explicit nwatcher(int fd, fs_handler handler = nullptr)
    : nio(fd), nstream(fd), handler_(handler) {}

    void set_handler(fs_handler handler) { handler_ = handler; }

    void add_watch(std::string path, uint32_t events);

    void del_watch(std::string path);

    void process_events();

private:
    std::unordered_map<int, std::string> wds_;

    std::unordered_map<std::string, int> paths_;

    fs_handler handler_;
};

#endif

class nio_factory {
public:
    static std::shared_ptr<nsocktcp> get_nsocktcp(family f);

    static std::shared_ptr<nsockudp> get_nsockudp(family f);

#ifdef __linux__
    static std::shared_ptr<nwatcher> get_nwatcher();
#endif
};

}

#endif