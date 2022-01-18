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
    unix
};

void set_nio(int fd);

int _query_family(family f);

class nio : public uncopyable {
public:
    explicit nio(int fd) : fd_(fd) {
        set_nio(fd_);
        rbuffer_ = std::unique_ptr<buffer>(new buffer(1));
        wbuffer_ = std::unique_ptr<buffer>(new buffer(1));
    }

    virtual ~nio() { close(fd_); }

    int fd() const { return fd_; }

    buffer *rbuf() { return rbuffer_.get(); }

    buffer *wbuf() { return wbuffer_.get(); }

protected:
    // File descriptor
    int fd_;

    // IO buffer, should be initialized
    std::unique_ptr<buffer> rbuffer_;

    std::unique_ptr<buffer> wbuffer_;
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
    int read_all(int step);

    // write until block or unwritable
    int write_all(int step);

protected:
    // Used by tcp-socket
    bool reset_;

    // End Of File: Used by tcp-socket, pipe, fifo, disk-file
    bool eof_;

    // Error Of Pipe: Used by tcp-socket, pipe, fifo
    bool eop_;

};


class nsock : public virtual nio {
public:
    nsock(int fd, family f) : nio(fd), family_(f) {}

    virtual ~nsock() {}

    int get_family() { return _query_family(family_); }

    std::tuple<std::string, int, family> sockname();

    std::tuple<std::string, int, family> peername();

    void set_reuse() {
        int optval = 1;
        socklen_t len = sizeof(optval);
        if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,  &optval, len) == -1)
        { throw_runtime_error("setsockopt error"); }
    }

    bool check_connect() {
        int optval;
        socklen_t len = sizeof(optval);
        if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &len) == -1)
        { throw_runtime_error("getsockopt error"); }
        return optval == 0;
    }

protected:
    // socket family
    family family_;
};


class nsocktcp final : public nsock, public nstream  {
public:
    nsocktcp(int sockfd, family f)
    : nio(sockfd), nsock(sockfd, f), nstream(sockfd) {}

    void listen(const int port, const char *ip = nullptr);

    void listen(const char *path);

    void connect(const char *ip, const int port);

    void connect(std::string ip, const int port)
    { connect(ip.c_str(), port); }

    void connect(const char *path);

    void connect(std::string path)
    { connect(path.c_str()); }

    std::vector<std::shared_ptr<nsocktcp> > accept(int batch = INT_MAX);

    std::tuple<std::string, int, family> connpeer() {
        return std::make_tuple<>(connect_peer_.first, connect_peer_.second, family_);
    }

private:
    std::pair<std::string, int> connect_peer_;
};


class nsockudp final : public nsock {
public:
    nsockudp(int sockfd, family f)
    : nio(sockfd), nsock(sockfd, f) {}

    void bind(const int port, const char *ip = nullptr);

    void bind(const char *path);

    std::tuple<std::string, int, family> recv();

    void send(const char *ip, const int port);

    void send(const char *path);
};

typedef void(*fs_handler)(inotify_event *, const char *path);

class nwatcher final : public nstream {
public:
    explicit nwatcher(int fd, fs_handler handler = nullptr)
    : nio(fd), nstream(fd), handler_(handler) {}

    void set_handler(fs_handler handler) { handler_ = handler; }

    void add_watch(std::string path, uint32_t events) {
        int wd = inotify_add_watch(fd_, path.c_str(), events);
        if (wd < 0) { throw_runtime_error("inotify_add_watch error"); }
        if (wds_.count(wd) == 0) { wds_[wd] = path; paths_[path] = wd; }
    }

    void del_watch(std::string path) {
        int wd = paths_[path];
        paths_.erase(path); wds_.erase(wd);
        if (inotify_rm_watch(fd_, paths_[path]) < 0)
        { throw_runtime_error("inotify_rm_watch error"); }
    }

    void process_events() {
        int len = read_chunk(sysconfig::inotify_step);
        char *p = rbuf()->buf();
        for (char *s = p; s < p + len; ++s) {
            inotify_event *evp = (inotify_event *)s;
            handler_(evp, wds_[evp->wd].c_str());
            p += sizeof(inotify_event) + evp->len;
        }
    }

private:
    std::unordered_map<int, std::string> wds_;

    std::unordered_map<std::string, int> paths_;

    fs_handler handler_;
};


class nio_factory {
public:
    static std::shared_ptr<nsocktcp> get_nsocktcp(family f) {
        int fd = ::socket(_query_family(f), SOCK_STREAM, 0);
        if (fd < 0) { throw_runtime_error("socket error"); }
        return std::shared_ptr<nsocktcp>(new nsocktcp(fd, f));
    }

    static std::shared_ptr<nsockudp> get_nsockudp(family f) {
        int fd = ::socket(_query_family(f), SOCK_DGRAM, 0);
        if (fd < 0) { throw_runtime_error("socket error"); }
        std::shared_ptr<nsockudp> sock(new nsockudp(fd, f));
        sock->rbuf()->resize(sysconfig::udp_buffer_size);
        sock->wbuf()->resize(sysconfig::udp_buffer_size);
        return sock;
    }

    static std::shared_ptr<nwatcher> get_nwatcher() {
        int fd = inotify_init();
        if (fd < 0) { throw_runtime_error("inotify_init error"); }
        return std::shared_ptr<nwatcher>(new nwatcher(fd));
    }

};

}

#endif