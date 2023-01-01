#include <unordered_set>
#include <fcntl.h>
#include <gtest/gtest.h>
#include "cppev/nio.h"
#include "cppev/event_loop.h"

namespace cppev
{

const char *file = "./cppev_test_file";

const char *fifo = "./cppev_test_fifo";

const char *str = "Cppev is a C++ event driven library";

class TestNio
: public testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(TestNio, test_diskfile)
{
    int fd;

    fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    std::shared_ptr<nstream> iofw(new nstream(fd));
    fd = open(file, O_RDONLY);
    std::shared_ptr<nstream> iofr(new nstream(fd));

    iofw->wbuf().put_string(str);
    iofw->write_all();
    iofw->close();
    iofr->read_all();
    EXPECT_STREQ(iofr->rbuf().rawbuf(), str);

    unlink(file);
}

TEST_F(TestNio, test_pipe)
{
    auto pipes = nio_factory::get_pipes();
    auto iopr = pipes[0];
    auto iopw = pipes[1];

    iopw->wbuf().put_string(str);
    iopw->write_all();
    iopr->read_all();
    EXPECT_STREQ(str, iopr->rbuf().rawbuf());
}

TEST_F(TestNio, test_fifo)
{
    auto fifos = nio_factory::get_fifos(fifo);
    auto iofr = fifos[0];
    auto iofw = fifos[1];
    iofw->wbuf().put_string(str);
    iofw->write_all();
    iofr->read_all();
    EXPECT_STREQ(iofr->rbuf().rawbuf(), str);

    unlink(fifo);
}


TEST_F(TestNio, test_tcp_connect_with_evlp)
{
    std::vector<std::tuple<family, int, std::string > > vec = {
        { family::ipv4, 8884, "127.0.0.1" },
        { family::ipv6, 8886, "::1"       },
    };

    int acpt_count = 0;
    int cont_count = 0;
    event_loop acpt_evlp(&acpt_count);
    event_loop cont_evlp(&cont_count);

    fd_event_handler cb = [](const std::shared_ptr<nio> &iop) -> void {
        (*reinterpret_cast<int *>(iop->evlp().data()))++;
    };

    for (size_t i = 0; i < vec.size(); ++i)
    {
        auto listensock = nio_factory::get_nsocktcp(std::get<0>(vec[i]));
        listensock->listen(std::get<1>(vec[i]));
        acpt_evlp.fd_register(std::dynamic_pointer_cast<nio>(listensock), 
            fd_event::fd_readable, cb);

        std::thread thr_cont([&]() {
            auto connsock = nio_factory::get_nsocktcp(std::get<0>(vec[i]));
            EXPECT_TRUE(connsock->connect(std::get<2>(vec[i]), std::get<1>(vec[i])));
            cont_evlp.fd_register(std::dynamic_pointer_cast<nio>(connsock), 
                fd_event::fd_writable, cb);
        });
        thr_cont.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    cont_evlp.loop_once();
    std::cout << "connecting loop finish" << std::endl;
    acpt_evlp.loop_once();
    std::cout << "listening loop finish" << std::endl;
    EXPECT_EQ(acpt_count, 2);
    EXPECT_EQ(cont_count, 2);
}



class TestNioSocket
: public testing::TestWithParam<std::tuple<family, bool, int, int> >
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
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

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}