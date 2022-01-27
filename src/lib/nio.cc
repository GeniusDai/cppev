#include "cppev/nio.h"
#include "cppev/sysconfig.h"
#include "cppev/common_utils.h"
#include <sys/socket.h>
#include <exception>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/un.h>
#include <climits>
#include <netinet/tcp.h>

namespace cppev {

void nio::set_nonblock() {
    int flags = fcntl(fd_, F_GETFL);
    if (flags < 0) { throw_system_error("fcntl error"); }
    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
    { throw_system_error("fcntl error"); }
}

const std::unordered_map<family, int, enum_hash> nsock::fmap_ = {
    {family::ipv4, AF_INET},
    {family::ipv6, AF_INET6},
    {family::local, AF_LOCAL}
};

const std::unordered_map<family, int, enum_hash> nsock::faddr_len_ = {
    {family::ipv4, sizeof(sockaddr_in)},
    {family::ipv6, sizeof(sockaddr_in6)}
};

static void set_ip_port(sockaddr_storage &addr, const char *ip, const int port) {
    switch (addr.ss_family) {
    case AF_INET: {
        sockaddr_in *ap = (sockaddr_in *)(&addr);
        ap->sin_port = htons(port);
        if (ip) {
            int rtn = inet_pton(addr.ss_family, ip, &(ap->sin_addr));
            if (rtn == 0) { throw_logic_error("inet_pton error"); }
            else if (rtn == -1) { throw_system_error("inet_pton error"); }
        }
        else { ap->sin_addr.s_addr = htonl(INADDR_ANY); }
        break;
    }
    case AF_INET6: {
        sockaddr_in6 *ap6 = (sockaddr_in6 *)(&addr);
        ap6->sin6_port = htons(port);
        if (ip) {
            int rtn = inet_pton(addr.ss_family, ip, &(ap6->sin6_addr));
            if (rtn == 0) { throw_logic_error("inet_pton error"); }
            else if (rtn == -1) { throw_system_error("inet_pton error"); }
        }
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
query_ip_port_family(sockaddr_storage &addr) {
    int port;
    char ip[sizeof(sockaddr_storage)];
    memset(ip, 0, sizeof(ip));
    family f;
    switch(addr.ss_family) {
    case AF_INET : {
        f = family::ipv4;
        sockaddr_in *ap = (sockaddr_in *)(&addr);
        port = ntohs(ap->sin_port);
        if (inet_ntop(ap->sin_family, &(ap->sin_addr), ip, sizeof(ip)) == nullptr)
        { throw_system_error("inet_ntop error"); }
        break;
    }
    case AF_INET6 : {
        f = family::ipv6;
        sockaddr_in6 *ap = (sockaddr_in6 *)(&addr);
        port = ntohs(ap->sin6_port);
        if (inet_ntop(ap->sin6_family, &(ap->sin6_addr), ip, sizeof(ip)) == nullptr)
        { throw_system_error("inet_ntop error"); }
        break;
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
            else { throw_system_error("read error"); }
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
            else { throw_system_error("write error"); }
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

void nsock::set_so_reuseaddr(bool enable) {
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,  &optval, len) == -1)
    { throw_system_error("setsockopt error"); }
}

void nsock::set_so_reuseport(bool enable) {
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT,  &optval, len) == -1)
    { throw_system_error("setsockopt error"); }
}

void nsock::set_so_rcvbuf(int size) {
    
}

void nsock::set_so_sndbuf(int size) {

}

void nsock::set_so_rcvlowat(int size) {

}

void nsock::set_so_sndlowat(int size) {

}

void nsocktcp::set_so_keepalive(bool enable) {
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE,  &optval, len) == -1)
    { throw_system_error("setsockopt error"); }
}

void nsocktcp::set_so_linger(bool l_onoff, int l_linger) {

}

void nsockudp::set_so_broadcast(bool enable) {
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_BROADCAST,  &optval, len) == -1)
    { throw_system_error("setsockopt error"); }
}

void nsocktcp::set_tcp_nodelay(bool enable) {
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY,  &optval, len) == -1)
    { throw_system_error("setsockopt error"); }
}

void nsocktcp::shutdown(shut_howto howto) {
    switch (howto) {
    case shut_howto::shut_rd : { ::shutdown(fd_, SHUT_RD); }
    case shut_howto::shut_wr : { ::shutdown(fd_, SHUT_WR); }
    case shut_howto::shut_rdwr : { ::shutdown(fd_, SHUT_RDWR); }
    default: { throw_logic_error("unknown shutdown howto"); }
    }
}

std::tuple<std::string, int, family> nsocktcp::sockname() {
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getsockname(fd_, (sockaddr *)&addr, &len) < 0)
    { throw_system_error("getsockname error"); }
    return query_ip_port_family(addr);
}

std::tuple<std::string, int, family> nsocktcp::peername() {
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getpeername(fd_, (sockaddr *)&addr, &len) < 0)
    { throw_system_error("getsockname error"); }
    return query_ip_port_family(addr);
}

bool nsocktcp::check_connect() {
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &len) == -1)
    { throw_system_error("getsockopt error"); }
    return optval == 0;
}

void nsocktcp::listen(const int port, const char *ip) {
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_ip_port(addr, ip, port);
    set_so_reuseaddr();
    if (::bind(fd_, (sockaddr *)&addr, faddr_len_.at(family_)) < 0)
    { throw_system_error("bind error"); }
    if (::listen(fd_, sysconfig::listen_number) < 0)
    { throw_system_error("listen error"); }
}

void nsocktcp::listen_unix(const char *path) {
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_path(addr, path);
    if (::bind(fd_, (sockaddr *)&addr, SUN_LEN((sockaddr_un *)&addr)) < 0)
    { throw_system_error("bind error"); }
    if (::listen(fd_, sysconfig::listen_number) < 0)
    { throw_system_error("listen error"); }
}

void nsocktcp::connect(const char *ip, const int port) {
    peer_ = std::make_pair<>(ip, port);
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_ip_port(addr, ip, port);
    if (::connect(fd_, (sockaddr *)&addr, faddr_len_.at(family_)) < 0 && errno != EINPROGRESS)
    { throw_system_error("connect error"); }
}

void nsocktcp::connect_unix(const char *path) {
    peer_ = std::make_pair<>(path, -1);
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_path(addr, path);
    int addr_len = SUN_LEN((sockaddr_un *)&addr);
    if (::connect(fd_, (sockaddr *)&addr, addr_len) < 0 && errno != EINPROGRESS)
    { throw_system_error("connect error"); }
}

std::vector<std::shared_ptr<nsocktcp> > nsocktcp::accept(int batch) {
    std::vector<std::shared_ptr<nsocktcp> > sockfds;
    for(int i = 0; i < batch; ++i) {
        int sockfd = ::accept(fd_, nullptr, nullptr);
        if (sockfd == -1) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) { break; }
            else { throw_system_error("accept error"); }
        } else {
            sockfds.emplace_back(new nsocktcp(sockfd, family_));
        }
    }
    return sockfds;
}

void nsockudp::bind(const int port, const char *ip) {
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_ip_port(addr, ip, port);
    set_so_reuseaddr();
    if (::bind(fd_, (sockaddr *)&addr, faddr_len_.at(family_)) < 0)
    { throw_system_error("bind error"); }
}

void nsockudp::bind_unix(const char *path) {
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_path(addr, path);
    if (::bind(fd_, (sockaddr *)&addr, SUN_LEN((sockaddr_un *)&addr)) < 0)
    { throw_system_error("bind error"); }
}

std::tuple<std::string, int, family> nsockudp::recv() {
    sockaddr_storage addr;
    socklen_t len = faddr_len_.at(family_);
    recvfrom(fd_, rbuf()->buffer_.get() + rbuf()->offset_,
        rbuf()->cap_ - rbuf()->offset_, 0, (sockaddr *)&addr, &len);
    return query_ip_port_family(addr);
}

void nsockudp::recv_unix() {
    recvfrom(fd_, rbuf()->buffer_.get() + rbuf()->offset_,
        rbuf()->cap_ - rbuf()->offset_, 0, nullptr, nullptr);
}

void nsockudp::send(const char *ip, const int port) {
    sockaddr_storage addr;
    addr.ss_family = fmap_.at(family_);
    set_ip_port(addr, ip, port);
    sendto(fd_, wbuf()->buffer_.get() + wbuf()->start_, wbuf()->size(),
        0, (sockaddr *)&addr, faddr_len_.at(family_));
}

void nsockudp::send_unix(const char *path) {
    sockaddr_storage addr;
    addr.ss_family = fmap_.at(family_);
    set_path(addr, path);
    sendto(fd_, wbuf()->buffer_.get() + wbuf()->start_, wbuf()->size(),
        0, (sockaddr *)&addr, SUN_LEN((sockaddr_un *)&addr));
}

std::shared_ptr<nsocktcp> nio_factory::get_nsocktcp(family f) {
    int fd = ::socket(nsock::fmap_.at(f), SOCK_STREAM, 0);
    if (fd < 0) { throw_system_error("socket error"); }
    return std::shared_ptr<nsocktcp>(new nsocktcp(fd, f));
}

std::shared_ptr<nsockudp> nio_factory::get_nsockudp(family f) {
    int fd = ::socket(nsock::fmap_.at(f), SOCK_DGRAM, 0);
    if (fd < 0) { throw_system_error("socket error"); }
    std::shared_ptr<nsockudp> sock(new nsockudp(fd, f));
    sock->rbuf()->resize(sysconfig::udp_buffer_size);
    sock->wbuf()->resize(sysconfig::udp_buffer_size);
    return sock;
}


// inotify for linux
#if defined(__linux__)

void nwatcher::add_watch(std::string path, uint32_t events) {
    int wd = inotify_add_watch(fd_, path.c_str(), events);
    if (wd < 0) { throw_system_error("inotify_add_watch error"); }
    if (wds_.count(wd) == 0) { wds_[wd] = path; paths_[path] = wd; }
}

void nwatcher::del_watch(std::string path) {
    int wd = paths_[path];
    paths_.erase(path); wds_.erase(wd);
    if (inotify_rm_watch(fd_, paths_[path]) < 0)
    { throw_system_error("inotify_rm_watch error"); }
}

void nwatcher::process_events() {
    int len = read_chunk(sysconfig::inotify_step);
    char *p = rbuf()->buf();
    for (char *s = p; s < p + len; ++s) {
        inotify_event *evp = (inotify_event *)s;
        handler_(evp, wds_[evp->wd].c_str());
        p += sizeof(inotify_event) + evp->len;
    }
}

std::shared_ptr<nwatcher> nio_factory::get_nwatcher() {
    int fd = inotify_init();
    if (fd < 0) { throw_system_error("inotify_init error"); }
    return std::shared_ptr<nwatcher>(new nwatcher(fd));
}

#endif

}
