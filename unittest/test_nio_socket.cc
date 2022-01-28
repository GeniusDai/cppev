#include <gtest/gtest.h>
#include "cppev/nio.h"

namespace cppev {

class TestNioSocket : public testing::TestWithParam<std::tuple<family, bool, int> > {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_P(TestNioSocket, test_tcp_socket) {
    auto p = GetParam();
    std::shared_ptr<nsocktcp> sock = nio_factory::get_nsocktcp(std::get<0>(p));
    sock->set_so_reuseaddr(std::get<1>(p));
    sock->set_so_reuseport(std::get<1>(p));
    sock->set_so_keepalive(std::get<1>(p));
    sock->set_so_linger(std::get<1>(p), std::get<2>(p));
    sock->set_tcp_nodelay(std::get<1>(p));
    EXPECT_EQ(sock->get_so_reuseaddr(), std::get<1>(p));
    EXPECT_EQ(sock->get_so_reuseport(), std::get<1>(p));
    EXPECT_EQ(sock->get_so_keepalive(), std::get<1>(p));
    EXPECT_EQ(sock->get_tcp_nodelay(), std::get<1>(p));
    auto lg = sock->get_so_linger();
    EXPECT_EQ(lg.first, std::get<1>(p));
    if (lg.first) { EXPECT_EQ(lg.second, std::get<2>(p)); }

    sock->set_so_rcvbuf(std::get<2>(p));
    sock->set_so_sndbuf(std::get<2>(p));
    // sock->set_so_sndlowat(std::get<2>(p));   // LINUX: Protocol not available
    sock->set_so_rcvlowat(std::get<2>(p));
    EXPECT_TRUE(sock->get_so_rcvbuf() == std::get<2>(p)
        || sock->get_so_rcvbuf() == std::get<2>(p) * 2);
    EXPECT_TRUE(sock->get_so_sndbuf() == std::get<2>(p)
        || sock->get_so_sndbuf() == std::get<2>(p) * 2);
    // EXPECT_EQ(sock->get_so_sndlowat(), std::get<2>(p));  // LINUX: Protocol not available
    EXPECT_EQ(sock->get_so_rcvlowat(), std::get<2>(p));
}

TEST_P(TestNioSocket, test_udp_socket) {
    auto p = GetParam();
    std::shared_ptr<nsockudp> sock = nio_factory::get_nsockudp(std::get<0>(p));
    sock->set_so_reuseaddr(std::get<1>(p));
    sock->set_so_reuseport(std::get<1>(p));
    sock->set_so_broadcast(std::get<1>(p));
    EXPECT_EQ(sock->get_so_reuseaddr(), std::get<1>(p));
    EXPECT_EQ(sock->get_so_reuseport(), std::get<1>(p));
    EXPECT_EQ(sock->get_so_broadcast(), std::get<1>(p));

    sock->set_so_rcvbuf(std::get<2>(p));
    sock->set_so_sndbuf(std::get<2>(p));
    // sock->set_so_sndlowat(std::get<2>(p));   // LINUX: Protocol not available
    sock->set_so_rcvlowat(std::get<2>(p));
    EXPECT_TRUE(sock->get_so_rcvbuf() == std::get<2>(p)
        || sock->get_so_rcvbuf() == std::get<2>(p) * 2);
    EXPECT_TRUE(sock->get_so_sndbuf() == std::get<2>(p)
        || sock->get_so_sndbuf() == std::get<2>(p) * 2);
    // EXPECT_EQ(sock->get_so_sndlowat(), std::get<2>(p));  // LINUX: Protocol not available
    EXPECT_EQ(sock->get_so_rcvlowat(), std::get<2>(p));
}

INSTANTIATE_TEST_SUITE_P(CppevTest, TestNioSocket,
    testing::Combine(
        testing::Values(family::ipv4, family::ipv6),
        testing::Bool(),
        testing::Values(8192, 16384, 32768)
    )
);

}

int main(int argc, char **argv) {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
