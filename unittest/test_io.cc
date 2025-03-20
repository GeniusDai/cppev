#include <fcntl.h>
#include <gtest/gtest.h>

#include <unordered_set>

#include "cppev/event_loop.h"
#include "cppev/io.h"

namespace cppev
{

const char *file = "./cppev_test_file";

const char *fifo = "./cppev_test_fifo";

const char *str = "Cppev is a C++ event driven library";

TEST(TestIO, test_diskfile)
{
    int fd;

    fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    auto iofw = std::make_shared<stream>(fd);
    fd = open(file, O_RDONLY);
    auto iofr = std::make_shared<stream>(fd);

    iofw->wbuffer().put_string(str);
    iofw->write_all();
    iofw->close();
    iofr->read_all();
    EXPECT_STREQ(iofr->rbuffer().data(), str);

    unlink(file);
}

TEST(TestIO, test_pipe)
{
    auto pipes = io_factory::get_pipes();
    auto iopr = pipes[0];
    auto iopw = pipes[1];

    iopw->wbuffer().put_string(str);
    iopw->write_all();
    iopr->read_all();
    EXPECT_STREQ(str, iopr->rbuffer().data());
}

TEST(TestIO, test_fifo)
{
    auto fifos = io_factory::get_fifos(fifo);
    auto iofr = fifos[0];
    auto iofw = fifos[1];
    iofw->wbuffer().put_string(str);
    iofw->write_all();
    iofr->read_all();
    EXPECT_STREQ(iofr->rbuffer().data(), str);

    unlink(fifo);
}

class TestIOSocket
    : public testing::TestWithParam<std::tuple<family, bool, int, int>>
{
};

TEST_P(TestIOSocket, test_tcp_socket)
{
    auto p = GetParam();
    std::shared_ptr<socktcp> sock = io_factory::get_socktcp(std::get<0>(p));
    sock->set_so_reuseaddr(std::get<1>(p));
    sock->set_so_reuseport(std::get<1>(p));
    sock->set_so_keepalive(std::get<1>(p));
    sock->set_so_linger(std::get<1>(p), std::get<3>(p));
    sock->set_tcp_nodelay(std::get<1>(p));
    EXPECT_EQ(sock->get_so_reuseaddr(), std::get<1>(p));
    EXPECT_EQ(sock->get_so_reuseport(), std::get<1>(p));
    EXPECT_EQ(sock->get_so_keepalive(), std::get<1>(p));
    EXPECT_EQ(sock->get_tcp_nodelay(), std::get<1>(p));
    auto lg = sock->get_so_linger();
    EXPECT_EQ(lg.first, std::get<1>(p));
    if (lg.first)
    {
        EXPECT_EQ(lg.second, std::get<3>(p));
    }

    sock->set_so_rcvbuf(std::get<2>(p));
    sock->set_so_sndbuf(std::get<2>(p));
    sock->set_so_rcvlowat(std::get<2>(p));

#ifndef __linux__
    sock->set_so_sndlowat(std::get<2>(p));
#endif

#ifdef __linux__
    int buf_size = std::get<2>(p) * 2;
    int sndlowat_size = 1;
#else
    int buf_size = std::get<2>(p);
    int sndlowat_size = std::get<2>(p);
#endif
    EXPECT_EQ(buf_size, sock->get_so_rcvbuf());
    EXPECT_EQ(buf_size, sock->get_so_sndbuf());
    EXPECT_EQ(sock->get_so_rcvlowat(), std::get<2>(p));
    EXPECT_EQ(sock->get_so_sndlowat(), sndlowat_size);

    EXPECT_EQ(sock->get_so_error(), 0);
}

TEST_P(TestIOSocket, test_udp_socket)
{
    auto p = GetParam();
    std::shared_ptr<sockudp> sock = io_factory::get_sockudp(std::get<0>(p));
    sock->set_so_reuseaddr(std::get<1>(p));
    sock->set_so_reuseport(std::get<1>(p));
    sock->set_so_broadcast(std::get<1>(p));
    EXPECT_EQ(sock->get_so_reuseaddr(), std::get<1>(p));
    EXPECT_EQ(sock->get_so_reuseport(), std::get<1>(p));
    EXPECT_EQ(sock->get_so_broadcast(), std::get<1>(p));

    sock->set_so_rcvbuf(std::get<2>(p));
    sock->set_so_sndbuf(std::get<2>(p));
    sock->set_so_rcvlowat(std::get<2>(p));
    std::unordered_set<int> buf_size{std::get<2>(p), std::get<2>(p) * 2};
    EXPECT_TRUE(buf_size.count(sock->get_so_rcvbuf()));
    EXPECT_TRUE(buf_size.count(sock->get_so_sndbuf()));
    EXPECT_EQ(sock->get_so_rcvlowat(), std::get<2>(p));
}

INSTANTIATE_TEST_SUITE_P(
    CppevTest, TestIOSocket,
    testing::Combine(
        testing::Values(family::ipv4, family::ipv6),  // protocol family
        testing::Bool(),                              // enable option
        testing::Values(8192, 16384, 32768),  // buffer or low water mark
        testing::Values(16, 32, 64, 128)      // linger-time
        ));

}  // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
