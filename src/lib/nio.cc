#include "cppev/nio.h"
#include "cppev/sysconfig.h"
#include "cppev/utils.h"
#include <cassert>
#include <sys/socket.h>
#include <exception>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/un.h>
#include <climits>
#include <netinet/tcp.h>
#include <tuple>
#include <sys/stat.h>
#include <vector>
#include <cstdio>

namespace cppev
{

void nio::close() noexcept
{
    if (fd_ != -1)
    {
        ::close(fd_);
    }
    closed_ = true;
}

void nio::set_io_nonblock()
{
    int flags = fcntl(fd_, F_GETFL);
    if (flags < 0)
    {
        throw_system_error("fcntl error");
    }
    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        throw_system_error("fcntl error");
    }
}

void nio::set_io_block()
{
    int flags = fcntl(fd_, F_GETFL);
    if (flags < 0)
    {
        throw_system_error("fcntl error");
    }
    if (fcntl(fd_, F_SETFL, flags & ~O_NONBLOCK) < 0)
    {
        throw_system_error("fcntl error");
    }
}

int nstream::read_chunk(int len)
{
    rbuffer().resize(rbuffer().offset_ + len);
    int origin_offset = rbuffer().offset_;
    while (len)
    {
        int curr = read(fd_, rbuffer().buffer_.get() + rbuffer().offset_, len);
        if (curr == 0)
        {
            eof_ = true;
            break;
        }
        if (curr == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else if (errno == ECONNRESET)
            {
                reset_ = true;
                break;
            }
            else
            {
                throw_system_error("read error");
            }
        }
        rbuffer().offset_ += curr;
        len -= curr;
    }
    return rbuffer().offset_ - origin_offset;
}

int nstream::write_chunk(int len)
{
    int origin_start = wbuffer().start_;
    len = std::min(len, wbuffer().size());
    while (len)
    {
        len = std::min(len, wbuffer().size());
        int curr = write(fd_, wbuffer().buffer_.get() + wbuffer().start_, len);
        if (curr == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else if (errno == EPIPE)
            {
                eop_ = true;
                break;
            }
            else if (errno == ECONNRESET)
            {
                reset_ = true;
                break;
            }
            else
            {
                throw_system_error("write error");
            }
        }
        wbuffer().start_ += curr;
        len -= curr;
    }
    int curr_start = wbuffer().start_;
    if (0 == wbuffer().size())
    {
        wbuffer().clear();
    }
    return curr_start - origin_start;
}

int nstream::read_all(int step)
{
    int total = 0;
    while(true)
    {
        int curr = read_chunk(step);
        total += curr;
        if (curr != step)
        {
            break;
        }
    }
    return total;
}

int nstream::write_all(int step)
{
    int total = 0;
    while(true)
    {
        int curr = write_chunk(step);
        total += curr;
        if (curr != step)
        {
            break;
        }
    }
    return total;
}

const std::unordered_map<family, int, enum_hash> nsock::fmap_ =
{
    {family::ipv4, AF_INET},
    {family::ipv6, AF_INET6},
    {family::local, AF_LOCAL}
};

const std::unordered_map<family, int, enum_hash> nsock::faddr_len_ =
{
    {family::ipv4, sizeof(sockaddr_in)},
    {family::ipv6, sizeof(sockaddr_in6)},
    {family::local, sizeof(sockaddr_un)}
};

static void set_ip_port(sockaddr_storage &addr, const char *ip, int port)
{
    switch (addr.ss_family)
    {
    case AF_INET :
    {
        sockaddr_in *ap = (sockaddr_in *)(&addr);
        ap->sin_port = htons(port);
        if (ip)
        {
            int rtn = inet_pton(addr.ss_family, ip, &(ap->sin_addr));
            if (rtn == 0)
            {
                throw_logic_error("inet_pton error");
            }
            else if (rtn == -1)
            {
                throw_system_error("inet_pton error");
            }
        }
        else
        {
            ap->sin_addr.s_addr = htonl(INADDR_ANY);
        }
        break;
    }
    case AF_INET6 :
    {
        sockaddr_in6 *ap6 = (sockaddr_in6 *)(&addr);
        ap6->sin6_port = htons(port);
        if (ip)
        {
            int rtn = inet_pton(addr.ss_family, ip, &(ap6->sin6_addr));
            if (rtn == 0)
            {
                throw_logic_error("inet_pton error");
            }
            else if (rtn == -1)
            {
                throw_system_error("inet_pton error");
            }
        }
        else
        {
            ap6->sin6_addr = in6addr_any;
        }
        break;
    }
    default:
    {
        throw_logic_error("unknown socket family");
    }
    }
}

static void set_path(sockaddr_storage &addr, const char *path)
{
    sockaddr_un *ap = (sockaddr_un *)(&addr);
    strncpy(ap->sun_path, path, sizeof(ap->sun_path) - 1);
}

static std::tuple<std::string, int, family> query_ip_port_family(sockaddr_storage &addr)
{
    int port;
    char ip[sizeof(sockaddr_storage)];
    memset(ip, 0, sizeof(ip));
    family f;
    switch(addr.ss_family)
    {
    case AF_INET :
    {
        f = family::ipv4;
        sockaddr_in *ap = (sockaddr_in *)(&addr);
        port = ntohs(ap->sin_port);
        if (inet_ntop(ap->sin_family, &(ap->sin_addr), ip, sizeof(ip)) == nullptr)
        {
            throw_system_error("inet_ntop error");
        }
        break;
    }
    case AF_INET6 :
    {
        f = family::ipv6;
        sockaddr_in6 *ap = (sockaddr_in6 *)(&addr);
        port = ntohs(ap->sin6_port);
        if (inet_ntop(ap->sin6_family, &(ap->sin6_addr), ip, sizeof(ip)) == nullptr)
        {
            throw_system_error("inet_ntop error");
        }
        break;
    }
    default:
    {
        throw_logic_error("unknown socket family");
    }
    }
    return std::make_tuple(ip, port, f);
}

void nsock::set_so_reuseaddr(bool enable)
{
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,  &optval, len) == -1)
    {
        throw_system_error("setsockopt error for SO_REUSEADDR");
    }
}

bool nsock::get_so_reuseaddr() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,  &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_REUSEADDR");
    }
    return static_cast<bool>(optval);
}

void nsock::set_so_reuseport(bool enable)
{
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT,  &optval, len) == -1)
    {
        throw_system_error("setsockopt error for SO_REUSEPORT");
    }
}

bool nsock::get_so_reuseport() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_REUSEPORT,  &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_REUSEPORT");
    }
    return static_cast<bool>(optval);
}

void nsock::set_so_rcvbuf(int size)
{
    if (setsockopt(fd_, SOL_SOCKET, SO_RCVBUF,  &size, sizeof(size)) == -1)
    {
        throw_system_error("setsockopt error for SO_RCVBUF");
    }
}

int nsock::get_so_rcvbuf() const
{
    int size;
    socklen_t len = sizeof(size);
    if (getsockopt(fd_, SOL_SOCKET, SO_RCVBUF,  &size, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_RCVBUF");
    }
    return size;
}

void nsock::set_so_sndbuf(int size)
{
    if (setsockopt(fd_, SOL_SOCKET, SO_SNDBUF,  &size, sizeof(size)) == -1)
    {
        throw_system_error("setsockopt error for SO_SNDBUF");
    }
}

int nsock::get_so_sndbuf() const
{
    int size;
    socklen_t len = sizeof(size);
    if (getsockopt(fd_, SOL_SOCKET, SO_SNDBUF,  &size, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_SNDBUF");
    }
    return size;
}

void nsock::set_so_rcvlowat(int size)
{
    if (setsockopt(fd_, SOL_SOCKET, SO_RCVLOWAT,  &size, sizeof(size)) == -1)
    {
        throw_system_error("setsockopt error for SO_RCVLOWAT");
    }
}

int nsock::get_so_rcvlowat() const
{
    int size;
    socklen_t len = sizeof(size);
    if (getsockopt(fd_, SOL_SOCKET, SO_RCVLOWAT,  &size, &len) == -1)
    { throw_system_error("getsockopt error for SO_RCVLOWAT"); }
    return size;
}

void nsock::set_so_sndlowat(int size)
{
    if (setsockopt(fd_, SOL_SOCKET, SO_SNDLOWAT,  &size, sizeof(size)) == -1)
    {
        throw_system_error("setsockopt error for SO_SNDLOWAT");
    }
}

int nsock::get_so_sndlowat() const
{
    int size;
    socklen_t len = sizeof(size);
    if (getsockopt(fd_, SOL_SOCKET, SO_SNDLOWAT,  &size, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_SNDLOWAT");
    }
    return size;
}

void nsocktcp::set_so_keepalive(bool enable)
{
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE,  &optval, len) == -1)
    {
        throw_system_error("setsockopt error for SO_KEEPALIVE");
    }
}

bool nsocktcp::get_so_keepalive() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE,  &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_KEEPALIVE");
    }
    return static_cast<bool>(optval);
}

void nsocktcp::set_so_linger(bool l_onoff, int l_linger)
{
    struct linger lg;
    lg.l_onoff = static_cast<int>(l_onoff);
    lg.l_linger = l_linger;
    if (setsockopt(fd_, SOL_SOCKET, SO_LINGER,  &lg, sizeof(lg)) == -1)
    {
        throw_system_error("setsockopt error for SO_LINGER");
    }
}

std::pair<bool, int> nsocktcp::get_so_linger() const
{
    struct linger lg;
    socklen_t len = sizeof(lg);
    if (getsockopt(fd_, SOL_SOCKET, SO_LINGER,  &lg, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_LINGER");
    }
    return std::make_pair<>(static_cast<bool>(lg.l_onoff), lg.l_linger);
}

void nsockudp::set_so_broadcast(bool enable)
{
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_BROADCAST,  &optval, len) == -1)
    {
        throw_system_error("setsockopt error for SO_BROADCAST");
    }
}

bool nsockudp::get_so_broadcast() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_BROADCAST,  &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_BROADCAST");
    }
    return static_cast<bool>(optval);
}

void nsocktcp::set_tcp_nodelay(bool enable)
{
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY,  &optval, len) == -1)
    {
        throw_system_error("setsockopt error for TCP_NODELAY");
    }
}

bool nsocktcp::get_tcp_nodelay() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for TCP_NODELAY");
    }
    return static_cast<bool>(optval);
}

int nsocktcp::get_so_error() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_ERROR");
    }
    return optval;
}

void nsocktcp::shutdown(shut_mode howto) noexcept
{
    switch (howto)
    {
    case shut_mode::shut_rd :
    {
        ::shutdown(fd_, SHUT_RD);
    }
    case shut_mode::shut_wr :
    {
        ::shutdown(fd_, SHUT_WR);
    }
    case shut_mode::shut_rdwr :
    {
        ::shutdown(fd_, SHUT_RDWR);
    }
    default: ;
    }
}

std::tuple<std::string, int, family> nsocktcp::sockname() const
{
    if (family_ == family::local)
    {
        return std::make_tuple(std::get<0>(peer_), std::get<1>(peer_), family::local);
    }
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getsockname(fd_, (sockaddr *)&addr, &len) < 0)
    {
        throw_system_error("getsockname error");
    }
    return query_ip_port_family(addr);
}

std::tuple<std::string, int, family> nsocktcp::peername() const
{
    if (family_ == family::local)
    {
        return std::make_tuple(std::get<0>(peer_), std::get<1>(peer_), family::local);
    }
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getpeername(fd_, (sockaddr *)&addr, &len) < 0)
    {
        throw_system_error("getsockname error");
    }
    return query_ip_port_family(addr);
}

void nsock::bind(const char *ip, int port)
{
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_ip_port(addr, ip, port);
    set_so_reuseaddr();
    if (::bind(fd_, (sockaddr *)&addr, faddr_len_.at(family_)) < 0)
    {
        throw_system_error(std::string("bind error : ").append(std::to_string(port)));
    }
}

void nsock::bind_unix(const char *path, bool remove)
{
    if (remove)
    {
        ::unlink(path);
    }
    peer_ = std::make_tuple(path, -1);
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_path(addr, path);
    if (::bind(fd_, (sockaddr *)&addr, SUN_LEN((sockaddr_un *)&addr)) < 0)
    {
        throw_system_error(std::string("bind error : ").append(path));
    }
}

void nsocktcp::listen(int backlog)
{
    if (::listen(fd_, backlog) < 0)
    {
        throw_system_error("listen error");
    }
}

bool nsocktcp::connect(const char *ip, int port)
{
    peer_ = std::make_tuple(ip, port);
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_ip_port(addr, ip, port);
    if (::connect(fd_, (sockaddr *)&addr, faddr_len_.at(family_)) < 0)
    {
        if (errno == EINPROGRESS)
        {
            return true;
        }
        return false;
    }
    return true;
}

bool nsocktcp::connect_unix(const char *path)
{
    peer_ = std::make_tuple(path, -1);
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_path(addr, path);
    int addr_len = SUN_LEN((sockaddr_un *)&addr);
    if (::connect(fd_, (sockaddr *)&addr, addr_len) < 0)
    {
        if (errno == EINPROGRESS)
        {
            return true;
        }
        return false;
    }
    return true;
}

std::vector<std::shared_ptr<nsocktcp>> nsocktcp::accept(int batch)
{
    std::vector<std::shared_ptr<nsocktcp>> sockfds;
    for(int i = 0; i < batch; ++i)
    {
        int sockfd = ::accept(fd_, nullptr, nullptr);
        if (sockfd == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            else
            {
                throw_system_error("accept error");
            }
        }
        else
        {
            sockfds.emplace_back(new nsocktcp(sockfd, family_));
            if (family_ == family::local)
            {
                sockfds.back()->peer_ = peer_;
            }
        }
    }
    return sockfds;
}

std::tuple<std::string, int, family> nsockudp::recv()
{
    sockaddr_storage addr;
    socklen_t len = faddr_len_.at(family_);
    int ret = recvfrom(fd_, rbuffer().buffer_.get() + rbuffer().offset_,
        rbuffer().cap_ - rbuffer().offset_, 0, (sockaddr *)&addr, &len);
    if (ret == 0)
    {
        throw_system_error("recvfrom error");
    }
    rbuffer().offset_ += ret;
    if (family_ == family::local)
    {
        return std::make_tuple(std::get<0>(peer_), -1, family::local);
    }
    return query_ip_port_family(addr);
}

void nsockudp::send(const char *ip, int port)
{
    sockaddr_storage addr;
    addr.ss_family = fmap_.at(family_);
    set_ip_port(addr, ip, port);
    int ret = sendto(fd_, wbuffer().buffer_.get() + wbuffer().start_, wbuffer().size(),
        0, (sockaddr *)&addr, faddr_len_.at(family_));
    if (ret == 0)
    {
        throw_system_error("sendto error");
    }
    wbuffer().consume(ret);
}

void nsockudp::send_unix(const char *path)
{
    sockaddr_storage addr;
    addr.ss_family = fmap_.at(family_);
    set_path(addr, path);
    int ret = sendto(fd_, wbuffer().buffer_.get() + wbuffer().start_, wbuffer().size(),
        0, (sockaddr *)&addr, SUN_LEN((sockaddr_un *)&addr));
    if (ret == 0)
    {
        throw_system_error("sendto error");
    }
    wbuffer().consume(ret);
}

namespace nio_factory
{

std::shared_ptr<nsocktcp> get_nsocktcp(family f)
{
    int fd = ::socket(nsock::fmap_.at(f), SOCK_STREAM, 0);
    if (fd < 0)
    {
        throw_system_error("socket error");
    }
    return std::make_shared<nsocktcp>(fd, f);
}

std::shared_ptr<nsockudp> get_nsockudp(family f)
{
    int fd = ::socket(nsock::fmap_.at(f), SOCK_DGRAM, 0);
    if (fd < 0)
    {
        throw_system_error("socket error");
    }
    std::shared_ptr<nsockudp> sock(new nsockudp(fd, f));
    sock->rbuffer().resize(sysconfig::udp_buffer_size);
    sock->wbuffer().resize(sysconfig::udp_buffer_size);
    return sock;
}

std::vector<std::shared_ptr<nstream>> get_pipes()
{
    int pfds[2];
    // pfds[0] refers to the read end of the pipe
    // pfds[1] refers to the write end of the pipe
    if (pipe(pfds) != 0)
    {
        throw_system_error("pipe error");
    }
    std::vector<std::shared_ptr<nstream>> pipes =
    {
        std::make_shared<nstream>(pfds[0]),
        std::make_shared<nstream>(pfds[1]),
    };
    return pipes;
}

std::vector<std::shared_ptr<nstream>> get_fifos(const std::string &path)
{
    if (mkfifo(path.c_str(), S_IRWXU) == -1 && errno != EEXIST)
    {
        throw_system_error("mkfifo error");
    }

    int fdr = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    if (fdr == -1)
    {
        throw_system_error("open error");
    }
    int fdw = open(path.c_str(), O_WRONLY | O_NONBLOCK);
    if (fdw == -1)
    {
        throw_system_error("open error");
    }
    std::vector<std::shared_ptr<nstream>> fifos =
    {
        std::make_shared<nstream>(fdr),
        std::make_shared<nstream>(fdw),
    };
    return fifos;
}

}   // namespace nio_factory

}   // namespace cppev
