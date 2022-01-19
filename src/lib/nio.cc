#include "cppev/nio.h"
#include "cppev/sysconfig.h"
#include <sys/socket.h>
#include <exception>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/un.h>
#include <climits>

namespace cppev {

void set_nio(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) { throw_runtime_error("fcntl error"); }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    { throw_runtime_error("fcntl error"); }
}

static const std::unordered_map<family, int, enum_hash> _fmap = {
    {family::ipv4, AF_INET},
    {family::ipv6, AF_INET6},
    {family::local, AF_LOCAL}
};

int _query_family(family f) { return _fmap.at(f); }

static void set_ip_port(sockaddr_storage &addr, const char *ip, const int port) {
    switch (addr.ss_family) {
    case AF_INET: {
        sockaddr_in *ap = (sockaddr_in *)(&addr);
        ap->sin_port = htons(port);
        if (ip) { inet_pton(addr.ss_family, ip, &(ap->sin_addr)); }
        else { ap->sin_addr.s_addr = htonl(INADDR_ANY); }
        break;
    }
    case AF_INET6: {
        sockaddr_in6 *ap6 = (sockaddr_in6 *)(&addr);
        ap6->sin6_port = htons(port);
        if (ip) { inet_pton(addr.ss_family, ip, &(ap6->sin6_addr)); }
        else { ap6->sin6_addr = in6addr_any; }
        break;
    }
    default: {
        throw_logic_error("unknown socket family");
    }
    }
}

static void set_path(sockaddr_storage &addr, const char *path) {
    sockaddr_un *ap = (sockaddr_un *)(&addr);
    strncpy(ap->sun_path, path, sizeof(ap->sun_path) - 1);
}

static std::tuple<std::string, int, family>
query_target(sockaddr_storage &addr) {
    int port;
    char ip[sizeof(sockaddr_storage)]; memset(ip, 0, 128);
    family f;
    switch(addr.ss_family) {
    case AF_INET : {
        f = family::ipv4;
        sockaddr_in *ap = (sockaddr_in *)(&addr);
        port = ntohs(ap->sin_port);
        if (inet_ntop(ap->sin_family, &addr, ip, sizeof(addr)) < 0)
        { throw_runtime_error("inet_ntop error"); }
        break;
    }
    case AF_INET6 : {
        f = family::ipv6;
        sockaddr_in6 *ap = (sockaddr_in6 *)(&addr);
        port = ntohs(ap->sin6_port);
        if (inet_ntop(ap->sin6_family, &addr, ip, sizeof(addr)) < 0)
        { throw_runtime_error("inet_ntop error"); }
        break;
    }
    case AF_LOCAL: {
        f = family::local;
        sockaddr_un *apu = (sockaddr_un *)&addr;
        port = -1;
        memcpy(ip,apu->sun_path, sizeof(apu->sun_path));
    }
    default: {
        throw_logic_error("unknown socket family");
    }
    }
    return std::make_tuple<>(std::string(ip), port, f);
}

int nstream::read_chunk(int len) {
    rbuffer_->resize(rbuffer_->offset_ + len);
    int origin_offset = rbuffer_->offset_;
    while (len) {
        int curr = read(fd_, rbuffer_->buffer_.get() + rbuffer_->offset_, len);
        if (curr == 0) { eof_ = true; break; }
        if (curr == -1) {
            if (errno == EINTR) { continue; }
            if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }
            else if (errno == ECONNRESET) { reset_ = true; break; }
            else { throw_runtime_error("read error"); }
        }
        rbuffer_->offset_ += curr;
        len -= curr;
    }
    return rbuffer_->offset_ - origin_offset;
}

int nstream::write_chunk(int len) {
    int origin_start = wbuffer_->start_;
    len = std::min(len, wbuffer_->size());
    while (len) {
        len = std::min(len, wbuffer_->size());
        int curr = write(fd_, wbuffer_->buffer_.get() + wbuffer_->start_, len);
        if (curr == -1) {
            if (errno == EINTR) { continue; }
            if (errno == EAGAIN || errno == EWOULDBLOCK) { break; }
            else if (errno == EPIPE) { eop_ = true; break; }
            else if (errno == ECONNRESET) { reset_ = true; break; }
            else { throw_runtime_error("write error"); }
        }
        wbuffer_->start_ += curr;
        len -= curr;
    }
    int curr_start = wbuffer_->start_;
    if (0 == wbuffer_->size()) { wbuffer_->clear(); }
    return curr_start - origin_start;
}

int nstream::read_all(int step) {
    int total = 0;
    while(true) {
        int curr = read_chunk(step);
        total += curr;
        if (curr != step) { break; }
    }
    return total;
}

int nstream::write_all(int step) {
    int total = 0;
    while(true) {
        int curr = write_chunk(step);
        total += curr;
        if (curr != step) { break; }
    }
    return total;
}

std::tuple<std::string, int, family>
nsock::sockname() {
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getsockname(fd_, (sockaddr *)&addr, &len) < 0)
    { throw_runtime_error("getsockname error"); }
    return query_target(addr);
}

std::tuple<std::string, int, family>
nsock::peername() {
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getpeername(fd_, (sockaddr *)&addr, &len) < 0)
    { throw_runtime_error("getsockname error"); }
    return query_target(addr);
}

void nsocktcp::listen(const int port, const char *ip) {
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = _query_family(family_);
    set_ip_port(addr, ip, port);
    set_reuse();
    if (::bind(fd_, (sockaddr *)&addr, sizeof(addr)) < 0)
    { throw_runtime_error("bind error"); }
    if (::listen(fd_, sysconfig::listen_number))
    { throw_runtime_error("listen error"); }
}

void nsocktcp::listen(const char *path) {
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = _query_family(family_);
    set_path(addr, path);
    if (::bind(fd_, (sockaddr *)&addr, SUN_LEN((sockaddr_un *)&addr)) < 0)
    { throw_runtime_error("bind error"); }
    if (::listen(fd_, sysconfig::listen_number) < 0)
    { throw_runtime_error("listen error"); }
}

void nsocktcp::connect(const char *ip, const int port) {
    connect_peer_ = std::make_pair<>(ip, port);
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = _query_family(family_);
    set_ip_port(addr, ip, port);
    if (::connect(fd_, (sockaddr *)&addr, sizeof(addr)) < 0 && errno != EINPROGRESS)
    { throw_runtime_error("connect error"); }
}

void nsocktcp::connect(const char *path) {
    connect_peer_ = std::make_pair<>(path, -1);
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = _query_family(family_);
    set_path(addr, path);
    int addr_len = SUN_LEN((sockaddr_un *)&addr);
    if (::connect(fd_, (sockaddr *)&addr, addr_len) < 0 && errno != EINPROGRESS)
    { throw_runtime_error("connect error"); }
}

std::vector<std::shared_ptr<nsocktcp> > nsocktcp::accept(int batch) {
    std::vector<std::shared_ptr<nsocktcp> > sockfds;
    for(int i = 0; i < batch; ++i) {
        int sockfd = ::accept(fd_, nullptr, nullptr);
        if (sockfd == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) { break; }
            else { throw_runtime_error("accept error"); }
        } else {
            sockfds.emplace_back(new nsocktcp(sockfd, family_));
        }
    }
    return sockfds;
}

void nsockudp::bind(const int port, const char *ip) {
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = _query_family(family_);
    set_ip_port(addr, ip, port);
    set_reuse();
    if (::bind(fd_, (sockaddr *)&addr, sizeof(addr)) < 0)
    { throw_runtime_error("bind error"); }
}

void nsockudp::bind(const char *path) {
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = _query_family(family_);
    set_path(addr, path);
    if (::bind(fd_, (sockaddr *)&addr, SUN_LEN((sockaddr_un *)&addr)) < 0)
    { throw_runtime_error("bind error"); }
}

std::tuple<std::string, int, family> nsockudp::recv() {
    sockaddr_storage addr;
    socklen_t len;
    recvfrom(fd_, rbuf()->buffer_.get() + rbuf()->offset_,
        rbuf()->cap_ - rbuf()->offset_, 0, (sockaddr *)&addr, &len);
    return query_target(addr);
}

void nsockudp::send(const char *ip, const int port) {
    sockaddr_storage addr;
    addr.ss_family = _query_family(family_);
    set_ip_port(addr, ip, port);
    sendto(fd_, wbuf()->buffer_.get() + wbuf()->start_, wbuf()->size(),
        0, (sockaddr *)&addr, sizeof(addr));
}

void nsockudp::send(const char *path) {
    sockaddr_storage addr;
    addr.ss_family = _query_family(family_);
    set_path(addr, path);
    sendto(fd_, wbuf()->buffer_.get() + wbuf()->start_, wbuf()->size(),
        0, (sockaddr *)&addr, sizeof(addr));
}

}
