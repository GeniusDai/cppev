#ifndef _nio_h_6C0224787A17_
#define _nio_h_6C0224787A17_

#include <sys/socket.h>
#include <sys/inotify.h>
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

namespace cppev {

class nio;
class nstream;
class nsock;
class nsockudp;
class nsocktcp;
class nwatcher;

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

protected:
    // File descriptor
    int fd_;

    // read buffer, should be initialized
    std::unique_ptr<buffer> rbuffer_;

    // write buffer, should be initialized
    std::unique_ptr<buffer> wbuffer_;

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

    void set_reuse();

protected:
    // socket family
    family family_;

    static const std::unordered_map<family, int, enum_hash> fmap_;

    static int query_family(family f) { return fmap_.at(f); }
};


class nsocktcp final : public nsock, public nstream  {
public:
    nsocktcp(int sockfd, family f)
    : nio(sockfd), nsock(sockfd, f), nstream(sockfd) {}

    void listen(const int port, const char *ip = nullptr);

    void listen(const int port, const std::string &ip) { listen(port, ip.c_str()); }

    void listen(const char *path);

    void listen(const std::string &path) { listen(path.c_str()); }

    void connect(const char *ip, const int port);

    void connect(const std::string &ip, const int port) { connect(ip.c_str(), port); }

    void connect(const char *path);

    void connect(const std::string &path) { connect(path.c_str()); }

    bool check_connect();

    std::vector<std::shared_ptr<nsocktcp> > accept(int batch = INT_MAX);

    std::tuple<std::string, int, family> sockname();

    std::tuple<std::string, int, family> peername();

    std::tuple<std::string, int, family> connpeer()
    {
        return std::make_tuple<>(connect_peer_.first,
            connect_peer_.second, family_);
    }

private:
    std::pair<std::string, int> connect_peer_;
};


class nsockudp final : public nsock {
public:
    nsockudp(int sockfd, family f)
    : nio(sockfd), nsock(sockfd, f) {}

    void bind(const int port, const char *ip = nullptr);

    void bind(const int port, const std::string &ip) { bind(port, ip.c_str()); }

    void bind(const char *path);

    void bind(const std::string &path) { bind(path.c_str()); }

    std::tuple<std::string, int, family> recv();

    void send(const char *ip, const int port);

    void send(const std::string &ip, const int port) { send(ip.c_str(), port); }

    void send(const char *path);

    void send(const std::string &path) { send(path.c_str()); }
};

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


class nio_factory {
public:
    static std::shared_ptr<nsocktcp> get_nsocktcp(family f);

    static std::shared_ptr<nsockudp> get_nsockudp(family f);

    static std::shared_ptr<nwatcher> get_nwatcher();
};

}

#endif