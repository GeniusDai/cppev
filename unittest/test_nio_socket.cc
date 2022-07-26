#include <gtest/gtest.h>
#include "cppev/nio.h"
#include <unordered_set>

namespace cppev {

class TestNioSocket
: public testing::TestWithParam<std::tuple<family, bool, int, int> >
{
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}
};

TEST_P(TestNioSocket, test_tcp_socket)
{
    auto p = GetParam();
    std::shared_ptr<nsocktcp> sock = nio_factory::get_nsocktcp(std::get<0>(p));
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
    if (lg.first) { EXPECT_EQ(lg.second, std::get<3>(p)); }

    sock->set_so_rcvbuf(std::get<2>(p));
    sock->set_so_sndbuf(std::get<2>(p));
    sock->set_so_rcvlowat(std::get<2>(p));
    std::unordered_set<int> buf_size{std::get<2>(p), std::get<2>(p)*2};
    EXPECT_TRUE(buf_size.count(sock->get_so_rcvbuf()));
    EXPECT_TRUE(buf_size.count(sock->get_so_sndbuf()));
    EXPECT_EQ(sock->get_so_rcvlowat(), std::get<2>(p));

    EXPECT_EQ(sock->get_so_error(), 0);
}

TEST_P(TestNioSocket, test_udp_socket)
{
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
    sock->set_so_rcvlowat(std::get<2>(p));
    std::unordered_set<int> buf_size{std::get<2>(p), std::get<2>(p)*2};
    EXPECT_TRUE(buf_size.count(sock->get_so_rcvbuf()));
    EXPECT_TRUE(buf_size.count(sock->get_so_sndbuf()));
    EXPECT_EQ(sock->get_so_rcvlowat(), std::get<2>(p));
}

INSTANTIATE_TEST_SUITE_P(CppevTest, TestNioSocket,
    testing::Combine(
        testing::Values(family::ipv4, family::ipv6),    // protocol family
        testing::Bool(),                                // enable option
        testing::Values(8192, 16384, 32768),            // buffer or low water mark
        testing::Values(16, 32, 64, 128)                // linger-time
    )
);

TEST_F(TestNioSocket, test_tcp_connect)
{
    std::vector<std::tuple<family, int, std::string > > vec = {
        { family::ipv4, 8884, "127.0.0.1" },
        { family::ipv6, 8886, "::1"       },
    };
    for (int i = 0; i < vec.size(); ++i)
    {
        auto listensock = nio_factory::get_nsocktcp(std::get<0>(vec[i]));
        listensock->listen(std::get<1>(vec[i]));

        std::thread thr_cont([&]() {
            auto connsock = nio_factory::get_nsocktcp(std::get<0>(vec[i]));
            EXPECT_TRUE(connsock->connect(std::get<2>(vec[i]), std::get<1>(vec[i])));
        });

        thr_cont.join();
    }
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
