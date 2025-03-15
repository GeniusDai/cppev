#include "cppev/io.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <cassert>
#include <climits>
#include <cstdio>
#include <cstring>
#include <exception>
#include <tuple>
#include <vector>

#include "cppev/common.h"
#include "cppev/utils.h"

namespace cppev
{

io::io(int fd, bool block) : fd_(fd), block_(block), closed_(false)
{
    if (!block)
    {
        set_io_nonblock();
    }
}

io::io(io &&other) noexcept
{
    if (&other == this)
    {
        return;
    }
    move(std::forward<io>(other));
}

io &io::operator=(io &&other) noexcept
{
    if (&other == this)
    {
        return *this;
    }
    move(std::forward<io>(other));
    return *this;
}

io::~io() noexcept
{
    if (!closed_)
    {
        close();
    }
}

int io::fd() const noexcept
{
    return fd_;
}

// Read buffer
const buffer &io::rbuffer() const noexcept
{
    return rbuffer_;
}

buffer &io::rbuffer() noexcept
{
    return rbuffer_;
}

// Write buffer
const buffer &io::wbuffer() const noexcept
{
    return wbuffer_;
}

buffer &io::wbuffer() noexcept
{
    return wbuffer_;
}

const event_loop &io::evlp() const noexcept
{
    return *evlp_;
}

event_loop &io::evlp() noexcept
{
    return *evlp_;
}

void io::set_evlp(event_loop *evlp) noexcept
{
    evlp_ = evlp;
}

bool io::is_closed() const noexcept
{
    return closed_;
}

void io::close() noexcept
{
    if (fd_ != -1)
    {
        ::close(fd_);
    }
    closed_ = true;
}

void io::set_io_nonblock()
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
    block_ = false;
}

void io::set_io_block()
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
    block_ = true;
}

void io::move(io &&other) noexcept
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

stream::stream(int fd) : io(fd), reset_(false), eof_(false), eop_(false)
{
}

stream::~stream() = default;

stream::stream(stream &&other) noexcept : io(std::forward<stream>(other))
{
    if (&other == this)
    {
        return;
    }
    move(std::forward<stream>(other), false);
}

stream &stream::operator=(stream &&other) noexcept
{
    if (&other == this)
    {
        return *this;
    }
    move(std::forward<stream>(other), true);
    return *this;
}

bool stream::is_reset() const noexcept
{
    return reset_;
}

bool stream::eof() const noexcept
{
    return eof_;
}

bool stream::eop() const noexcept
{
    return eop_;
}

int stream::read_chunk(int len)
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

int stream::write_chunk(int len)
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

int stream::read_all(int step)
{
    if (this->block_)
    {
        throw_logic_error("block io shall never call read_all");
    }
    int total = 0;
    while (true)
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

int stream::write_all(int step)
{
    if (this->block_)
    {
        throw_logic_error("block io shall never call write_all");
    }
    int total = 0;
    while (true)
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

void stream::move(stream &&other, bool move_base) noexcept
{
    if (move_base)
    {
        io::move(std::forward<stream>(other));
    }
    this->reset_ = other.reset_;
    this->eof_ = other.eof_;
    this->eop_ = other.eop_;
}

static const std::unordered_map<family, int, enum_hash> fmap_ = {
    {family::ipv4, AF_INET},
    {family::ipv6, AF_INET6},
    {family::local, AF_LOCAL}};

static const std::unordered_map<family, int, enum_hash> faddr_len_ = {
    {family::ipv4, sizeof(sockaddr_in)},
    {family::ipv6, sizeof(sockaddr_in6)},
    {family::local, sizeof(sockaddr_un)}};

static void set_inet_uri(sockaddr_storage &addr, const char *ip, int port)
{
    switch (addr.ss_family)
    {
    case AF_INET:
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
    case AF_INET6:
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
        throw_logic_error("unknown socket family");
    }
}

static void set_unix_uri(sockaddr_storage &addr, const char *path)
{
    sockaddr_un *ap = (sockaddr_un *)(&addr);
    strncpy(ap->sun_path, path, sizeof(ap->sun_path) - 1);
}

static std::tuple<std::string, int, family> get_inet_uri(
    const sockaddr_storage &addr)
{
    int port;
    char ip[sizeof(sockaddr_storage)];
    memset(ip, 0, sizeof(ip));
    family f;
    switch (addr.ss_family)
    {
    case AF_INET:
        {
            f = family::ipv4;
            sockaddr_in *ap = (sockaddr_in *)(&addr);
            port = ntohs(ap->sin_port);
            if (inet_ntop(ap->sin_family, &(ap->sin_addr), ip, sizeof(ip)) ==
                nullptr)
            {
                throw_system_error("inet_ntop error");
            }
            break;
        }
    case AF_INET6:
        {
            f = family::ipv6;
            sockaddr_in6 *ap = (sockaddr_in6 *)(&addr);
            port = ntohs(ap->sin6_port);
            if (inet_ntop(ap->sin6_family, &(ap->sin6_addr), ip, sizeof(ip)) ==
                nullptr)
            {
                throw_system_error("inet_ntop error");
            }
            break;
        }
    default:
        throw_logic_error("unknown socket family");
    }
    return std::make_tuple(ip, port, f);
}

sock::sock(int fd, family f) : io(fd), family_(f)
{
}

sock::~sock() = default;

sock::sock(sock &&other) noexcept : io(std::forward<sock>(other))
{
    if (&other == this)
    {
        return;
    }
    move(std::forward<sock>(other), false);
}

sock &sock::operator=(sock &&other) noexcept
{
    if (&other == this)
    {
        return *this;
    }
    move(std::forward<sock>(other), true);
    return *this;
}

family sock::sockfamily() const noexcept
{
    return family_;
}

void sock::bind(const char *ip, int port)
{
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_inet_uri(addr, ip, port);
    set_so_reuseaddr();
    if (::bind(fd_, (sockaddr *)&addr, faddr_len_.at(family_)) < 0)
    {
        throw_system_error(
            std::string("bind error : ").append(std::to_string(port)));
    }
}

void sock::bind(int port)
{
    bind(nullptr, port);
}

void sock::bind(const std::string &ip, int port)
{
    bind(ip.c_str(), port);
}

void sock::bind_unix(const char *path, bool remove)
{
    if (remove)
    {
        ::unlink(path);
    }
    unix_path_ = path;
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_unix_uri(addr, path);
    if (::bind(fd_, (sockaddr *)&addr, SUN_LEN((sockaddr_un *)&addr)) < 0)
    {
        throw_system_error(std::string("bind error : ").append(path));
    }
}

void sock::bind_unix(const std::string &path, bool remove)
{
    bind_unix(path.c_str(), remove);
}

void sock::set_so_reuseaddr(bool enable)
{
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, len) == -1)
    {
        throw_system_error("setsockopt error for SO_REUSEADDR");
    }
}

bool sock::get_so_reuseaddr() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_REUSEADDR");
    }
    return static_cast<bool>(optval);
}

void sock::set_so_reuseport(bool enable)
{
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, len) == -1)
    {
        throw_system_error("setsockopt error for SO_REUSEPORT");
    }
}

bool sock::get_so_reuseport() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_REUSEPORT");
    }
    return static_cast<bool>(optval);
}

void sock::set_so_rcvbuf(int size)
{
    if (setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) == -1)
    {
        throw_system_error("setsockopt error for SO_RCVBUF");
    }
}

int sock::get_so_rcvbuf() const
{
    int size;
    socklen_t len = sizeof(size);
    if (getsockopt(fd_, SOL_SOCKET, SO_RCVBUF, &size, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_RCVBUF");
    }
    return size;
}

void sock::set_so_sndbuf(int size)
{
    if (setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) == -1)
    {
        throw_system_error("setsockopt error for SO_SNDBUF");
    }
}

int sock::get_so_sndbuf() const
{
    int size;
    socklen_t len = sizeof(size);
    if (getsockopt(fd_, SOL_SOCKET, SO_SNDBUF, &size, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_SNDBUF");
    }
    return size;
}

void sock::set_so_rcvlowat(int size)
{
    if (setsockopt(fd_, SOL_SOCKET, SO_RCVLOWAT, &size, sizeof(size)) == -1)
    {
        throw_system_error("setsockopt error for SO_RCVLOWAT");
    }
}

int sock::get_so_rcvlowat() const
{
    int size;
    socklen_t len = sizeof(size);
    if (getsockopt(fd_, SOL_SOCKET, SO_RCVLOWAT, &size, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_RCVLOWAT");
    }
    return size;
}

void sock::set_so_sndlowat(int size)
{
    if (setsockopt(fd_, SOL_SOCKET, SO_SNDLOWAT, &size, sizeof(size)) == -1)
    {
        throw_system_error("setsockopt error for SO_SNDLOWAT");
    }
}

int sock::get_so_sndlowat() const
{
    int size;
    socklen_t len = sizeof(size);
    if (getsockopt(fd_, SOL_SOCKET, SO_SNDLOWAT, &size, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_SNDLOWAT");
    }
    return size;
}

void sock::move(sock &&other, bool move_base) noexcept
{
    if (move_base)
    {
        io::move(std::forward<sock>(other));
    }
    this->family_ = other.family_;
    this->unix_path_ = other.unix_path_;
}

socktcp::socktcp(int sockfd, family f) : io(sockfd), sock(-1, f), stream(-1)
{
}

socktcp::~socktcp() = default;

socktcp::socktcp(socktcp &&other) noexcept
    : io(std::forward<socktcp>(other)),
      sock(std::forward<socktcp>(other)),
      stream(std::forward<socktcp>(other))
{
    if (&other == this)
    {
        return;
    }
    move(std::forward<socktcp>(other), false);
}

socktcp &socktcp::operator=(socktcp &&other) noexcept
{
    if (&other == this)
    {
        return *this;
    }
    move(std::forward<socktcp>(other), true);
    return *this;
}

bool socktcp::connect(const std::string &ip, int port)
{
    return connect(ip.c_str(), port);
}

bool socktcp::connect_unix(const std::string &path)
{
    return connect_unix(path.c_str());
}

bool socktcp::check_connect() const
{
    return get_so_error() == 0;
}

void socktcp::set_so_keepalive(bool enable)
{
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, len) == -1)
    {
        throw_system_error("setsockopt error for SO_KEEPALIVE");
    }
}

bool socktcp::get_so_keepalive() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_KEEPALIVE");
    }
    return static_cast<bool>(optval);
}

void socktcp::set_so_linger(bool l_onoff, int l_linger)
{
    struct linger lg;
    lg.l_onoff = static_cast<int>(l_onoff);
    lg.l_linger = l_linger;
    if (setsockopt(fd_, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg)) == -1)
    {
        throw_system_error("setsockopt error for SO_LINGER");
    }
}

std::pair<bool, int> socktcp::get_so_linger() const
{
    struct linger lg;
    socklen_t len = sizeof(lg);
    if (getsockopt(fd_, SOL_SOCKET, SO_LINGER, &lg, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_LINGER");
    }
    return std::make_pair<>(static_cast<bool>(lg.l_onoff), lg.l_linger);
}

void sockudp::set_so_broadcast(bool enable)
{
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, SOL_SOCKET, SO_BROADCAST, &optval, len) == -1)
    {
        throw_system_error("setsockopt error for SO_BROADCAST");
    }
}

bool sockudp::get_so_broadcast() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_BROADCAST, &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_BROADCAST");
    }
    return static_cast<bool>(optval);
}

void socktcp::set_tcp_nodelay(bool enable)
{
    int optval = static_cast<int>(enable);
    socklen_t len = sizeof(optval);
    if (setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, len) == -1)
    {
        throw_system_error("setsockopt error for TCP_NODELAY");
    }
}

bool socktcp::get_tcp_nodelay() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for TCP_NODELAY");
    }
    return static_cast<bool>(optval);
}

int socktcp::get_so_error() const
{
    int optval;
    socklen_t len = sizeof(optval);
    if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &optval, &len) == -1)
    {
        throw_system_error("getsockopt error for SO_ERROR");
    }
    return optval;
}

void socktcp::shutdown(shutdown_mode howto) noexcept
{
    switch (howto)
    {
    case shutdown_mode::shutdown_rd:
        {
            ::shutdown(fd_, SHUT_RD);
            break;
        }
    case shutdown_mode::shutdown_wr:
        {
            ::shutdown(fd_, SHUT_WR);
            break;
        }
    case shutdown_mode::shutdown_rdwr:
        {
            ::shutdown(fd_, SHUT_RDWR);
            break;
        }
    default:;
    }
}

std::tuple<std::string, int, family> socktcp::sockname() const
{
    if (family_ == family::local)
    {
        return std::make_tuple(unix_path_, -1, family::local);
    }
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getsockname(fd_, (sockaddr *)&addr, &len) < 0)
    {
        throw_system_error("getsockname error");
    }
    return get_inet_uri(addr);
}

std::tuple<std::string, int, family> socktcp::peername() const
{
    if (family_ == family::local)
    {
        return std::make_tuple(unix_path_, -1, family::local);
    }
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    if (getpeername(fd_, (sockaddr *)&addr, &len) < 0)
    {
        throw_system_error("getsockname error");
    }
    return get_inet_uri(addr);
}

std::tuple<std::string, int, family> socktcp::target_uri() const noexcept
{
    return std::make_tuple(std::get<0>(conn_uri_), std::get<1>(conn_uri_),
                           family_);
}

void socktcp::listen(int backlog)
{
    if (::listen(fd_, backlog) < 0)
    {
        throw_system_error("listen error");
    }
}

bool socktcp::connect(const char *ip, int port)
{
    conn_uri_ = std::make_tuple(ip, port);
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_inet_uri(addr, ip, port);
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

bool socktcp::connect_unix(const char *path)
{
    conn_uri_ = std::make_tuple(path, -1);
    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    addr.ss_family = fmap_.at(family_);
    set_unix_uri(addr, path);
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

std::vector<std::shared_ptr<socktcp>> socktcp::accept(int batch)
{
    std::vector<std::shared_ptr<socktcp>> sockfds;
    for (int i = 0; i < batch; ++i)
    {
        int sockfd = ::accept(fd_, nullptr, nullptr);
        if (sockfd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
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
            sockfds.emplace_back(std::make_shared<socktcp>(sockfd, family_));
            if (family_ == family::local)
            {
                sockfds.back()->unix_path_ = unix_path_;
            }
        }
    }
    return sockfds;
}

void socktcp::move(socktcp &&other, bool move_base) noexcept
{
    if (move_base)
    {
        io::move(std::forward<socktcp>(other));
        sock::move(std::forward<socktcp>(other), false);
        stream::move(std::forward<socktcp>(other), false);
    }
    this->conn_uri_ = other.conn_uri_;
}

sockudp::sockudp(int sockfd, family f) : io(sockfd), sock(-1, f)
{
}

sockudp::~sockudp() = default;

sockudp::sockudp(sockudp &&other) noexcept
    : io(std::forward<sockudp>(other)), sock(std::forward<sockudp>(other))
{
    if (&other == this)
    {
        return;
    }
    move(std::forward<sockudp>(other), false);
}

sockudp &sockudp::operator=(sockudp &&other) noexcept
{
    if (&other == this)
    {
        return *this;
    }
    move(std::forward<sockudp>(other), true);
    return *this;
}

void sockudp::send(const std::string &ip, int port)
{
    send(ip.c_str(), port);
}

void sockudp::send_unix(const std::string &path)
{
    send_unix(path.c_str());
}

std::tuple<std::string, int, family> sockudp::recv()
{
    sockaddr_storage addr;
    socklen_t len = faddr_len_.at(family_);
    int ret = recvfrom(fd_, rbuffer().buffer_.get() + rbuffer().offset_,
                       rbuffer().cap_ - rbuffer().offset_, 0, (sockaddr *)&addr,
                       &len);
    if (ret == 0)
    {
        throw_system_error("recvfrom error");
    }
    rbuffer().offset_ += ret;
    if (family_ == family::local)
    {
        return std::make_tuple(unix_path_, -1, family::local);
    }
    return get_inet_uri(addr);
}

void sockudp::send(const char *ip, int port)
{
    sockaddr_storage addr;
    addr.ss_family = fmap_.at(family_);
    set_inet_uri(addr, ip, port);
    int ret =
        sendto(fd_, wbuffer().buffer_.get() + wbuffer().start_,
               wbuffer().size(), 0, (sockaddr *)&addr, faddr_len_.at(family_));
    if (ret == 0)
    {
        throw_system_error("sendto error");
    }
    wbuffer().consume(ret);
}

void sockudp::send_unix(const char *path)
{
    sockaddr_storage addr;
    addr.ss_family = fmap_.at(family_);
    set_unix_uri(addr, path);
    int ret = sendto(fd_, wbuffer().buffer_.get() + wbuffer().start_,
                     wbuffer().size(), 0, (sockaddr *)&addr,
                     SUN_LEN((sockaddr_un *)&addr));
    if (ret == 0)
    {
        throw_system_error("sendto error");
    }
    wbuffer().consume(ret);
}

void sockudp::move(sockudp &&other, bool move_base) noexcept
{
    if (move_base)
    {
        sock::move(std::forward<sockudp>(other), true);
    }
}

namespace io_factory
{

std::shared_ptr<socktcp> get_socktcp(family f)
{
    int fd = ::socket(fmap_.at(f), SOCK_STREAM, 0);
    if (fd < 0)
    {
        throw_system_error("socket error");
    }
    return std::make_shared<socktcp>(fd, f);
}

std::shared_ptr<sockudp> get_sockudp(family f)
{
    int fd = ::socket(fmap_.at(f), SOCK_DGRAM, 0);
    if (fd < 0)
    {
        throw_system_error("socket error");
    }
    std::shared_ptr<sockudp> sock = std::make_shared<sockudp>(fd, f);
    sock->rbuffer().resize(sysconfig::udp_buffer_size);
    sock->wbuffer().resize(sysconfig::udp_buffer_size);
    return sock;
}

std::vector<std::shared_ptr<stream>> get_pipes()
{
    int pfds[2];
    // pfds[0] refers to the read end of the pipe
    // pfds[1] refers to the write end of the pipe
    if (pipe(pfds) != 0)
    {
        throw_system_error("pipe error");
    }
    std::vector<std::shared_ptr<stream>> pipes = {
        std::make_shared<stream>(pfds[0]),
        std::make_shared<stream>(pfds[1]),
    };
    return pipes;
}

std::vector<std::shared_ptr<stream>> get_fifos(const std::string &path)
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
    std::vector<std::shared_ptr<stream>> fifos = {
        std::make_shared<stream>(fdr),
        std::make_shared<stream>(fdw),
    };
    return fifos;
}

}  // namespace io_factory

}  // namespace cppev
