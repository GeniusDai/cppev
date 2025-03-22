#include <fcntl.h>
#include <gtest/gtest.h>

#include <unordered_set>

#include "cppev/event_loop.h"
#include "cppev/io.h"
#include "cppev/logger.h"

namespace cppev
{

TEST(TestEvlpEnumClass, test_enum_class_operator_or)
{
    fd_event fd1 = fd_event::fd_readable;
    fd_event fd2 = fd_event::fd_writable;
    fd_event fd3 = fd1 | fd2;
    fd2 |= fd1;
    ASSERT_EQ(fd3, fd2);
    ASSERT_NE(fd3, fd1);
}

TEST(TestEvlpEnumClass, test_enum_class_operator_and)
{
    fd_event fd1 = fd_event::fd_readable;
    fd_event fd2 = fd_event::fd_writable;
    fd_event fd3 = fd1 & fd2;
    fd2 &= fd1;
    ASSERT_EQ(fd3, fd2);
}

TEST(TestEvlpEnumClass, test_enum_class_operator_xor)
{
    fd_event fd1 = fd_event::fd_readable;
    fd_event fd2 = fd_event::fd_writable;
    fd_event fd3 = fd1 ^ fd2;
    fd_event fd4 = fd1 | fd2;
    fd2 ^= fd1;
    ASSERT_EQ(fd3, fd4);
    ASSERT_EQ(fd3, fd2);
    fd2 ^= fd1;
    ASSERT_EQ(fd1 ^ fd2, fd4);
    ASSERT_EQ(fd4 ^ fd2, fd1);
    ASSERT_EQ(fd4 ^ fd1, fd2);
}

const char *str = "Cppev is a C++ event driven library";

class TestEventLoop : public testing::TestWithParam<fd_event_mode>
{
};

TEST_P(TestEventLoop, test_tcp_connect_with_evlp_first)
{
    cppev::logger::get_instance().set_log_level(cppev::log_level::info);

    std::vector<std::tuple<family, int, std::string>> vec = {
        {family::ipv4, 8884, "127.0.0.1"},
        {family::ipv6, 8886, "::1"},
    };

    int acpt_count = 0;
    event_loop acpt_evlp(&acpt_count);

    int cont_count = 0;
    event_loop cont_evlp(&cont_count);

    auto p = GetParam();

    fd_event_handler acpt_writable_callback =
        [p](const std::shared_ptr<io> &iop)
    {
        auto dp = reinterpret_cast<int *>(iop->evlp().data());
        dp ? ++(*dp) : 0;
        auto conns = std::dynamic_pointer_cast<socktcp>(iop)->accept();
        for (auto conn : conns)
        {
            iop->evlp().fd_set_mode(conn, p);
            iop->evlp().fd_register_and_activate(
                std::static_pointer_cast<io>(conn), fd_event::fd_writable,
                [](const std::shared_ptr<io> &iop)
                {
                    LOG_INFO_FMT(
                        "Server side connected socket %d writable event "
                        "triggered",
                        iop->fd());
                    auto iopt = std::dynamic_pointer_cast<socktcp>(iop);
                    iopt->wbuffer().put_string(str);
                    iopt->write_all();
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                });
        }
    };

    for (size_t i = 0; i < vec.size(); ++i)
    {
        // Test Event Loop API
        auto listesock = io_factory::get_socktcp(std::get<0>(vec[i]));
        listesock->bind(std::get<1>(vec[i]));
        listesock->listen();
        auto acpt_iop = std::dynamic_pointer_cast<io>(listesock);
        acpt_evlp.fd_register_and_activate(acpt_iop, fd_event::fd_readable,
                                           acpt_writable_callback);
        acpt_evlp.fd_clean(acpt_iop);
        acpt_evlp.fd_register_and_activate(acpt_iop, fd_event::fd_readable,
                                           acpt_writable_callback);
    }

    std::thread thr_cont(
        [&]()
        {
            for (size_t i = 0; i < vec.size(); ++i)
            {
                auto consock = io_factory::get_socktcp(std::get<0>(vec[i]));
                EXPECT_TRUE(
                    consock->connect(std::get<2>(vec[i]), std::get<1>(vec[i])));
                auto conn_iop = std::dynamic_pointer_cast<io>(consock);
                cont_evlp.fd_register(
                    conn_iop, fd_event::fd_writable,
                    [](const std::shared_ptr<io> &iop)
                    {
                        LOG_INFO_FMT(
                            "Client side connected socket %d writable event "
                            "triggered",
                            iop->fd());
                        auto dp = reinterpret_cast<int *>(iop->evlp().data());
                        dp ? ++(*dp) : 0;
                    });
                cont_evlp.fd_activate(conn_iop, fd_event::fd_writable);
                cont_evlp.fd_deactivate(conn_iop, fd_event::fd_writable);
                cont_evlp.fd_activate(conn_iop, fd_event::fd_writable);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            cont_evlp.loop_once();
        });
    thr_cont.join();

    std::thread thr_stop(
        [&]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            acpt_evlp.stop_loop();
        });

    acpt_evlp.loop_forever();
    thr_stop.join();

    EXPECT_EQ(acpt_count, vec.size());
    EXPECT_EQ(cont_count, vec.size());

    LOG_INFO << "server client test ended";

    std::thread thr1([&]() { acpt_evlp.loop_once(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    acpt_evlp.stop_loop();
    thr1.join();
    LOG_INFO << "loop once stopped";

    std::thread thr2([&]() { acpt_evlp.loop_forever(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    acpt_evlp.stop_loop();
    thr2.join();
    LOG_INFO << "loop forever stopped";
}

TEST_P(TestEventLoop, test_tcp_connect_with_evlp_second)
{
    cppev::logger::get_instance().set_log_level(cppev::log_level::info);

    const std::string path = "/tmp/unittest_cppev_tcp_unix_6C0224787A17.sock";
    const int port = 8889;

    event_loop client_evlp;

    auto p = GetParam();

    fd_event_handler conn_callback = [p](const std::shared_ptr<io> &iop)
    {
        LOG_INFO << "Executing fd " << iop->fd() << " fd_writable callback";
        iop->evlp().fd_set_mode(iop, p);
        iop->evlp().fd_deactivate(iop, fd_event::fd_writable);
        iop->evlp().fd_register_and_activate(
            std::dynamic_pointer_cast<io>(iop), fd_event::fd_readable,
            [](const std::shared_ptr<io> &iop)
            {
                LOG_INFO << "Executing fd " << iop->fd()
                         << " fd_readable callback";
                auto iopt = std::dynamic_pointer_cast<socktcp>(iop);
                iopt->read_chunk(8);
                if (iop->rbuffer().size() != strlen(str))
                {
                    return;
                }
                LOG_INFO << "Executing fd " << iop->fd()
                         << " received message : "
                         << iop->rbuffer().get_string();
            });
    };

    auto listesock1 = io_factory::get_socktcp(family::local);
    listesock1->bind_unix(path, true);
    listesock1->listen();

    auto listesock2 = io_factory::get_socktcp(family::ipv6);
    listesock2->bind(port);
    listesock2->listen();

    std::thread sub_thr(
        [&]()
        {
            auto consock1 = io_factory::get_socktcp(family::local);
            bool succeed = consock1->connect_unix(path);
            ASSERT_TRUE(succeed);

            auto consock2 = io_factory::get_socktcp(family::ipv6);
            succeed = consock2->connect("::1", port);
            ASSERT_TRUE(succeed);

            client_evlp.fd_register_and_activate(
                std::dynamic_pointer_cast<io>(consock1), fd_event::fd_writable,
                conn_callback);
            client_evlp.fd_register_and_activate(
                std::dynamic_pointer_cast<io>(consock2), fd_event::fd_writable,
                conn_callback);
            client_evlp.loop_forever();
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    auto conns1 = listesock1->accept();
    EXPECT_TRUE(conns1.size() == 1);
    auto consock1 = conns1[0];

    auto conns2 = listesock2->accept();
    EXPECT_TRUE(conns2.size() == 1);
    auto consock2 = conns2[0];

    for (int i = 0; i < 2; ++i)
    {
        consock1->wbuffer().put_string(str);
        consock1->write_all();
        consock2->wbuffer().put_string(str);
        consock2->write_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOG_INFO << "To leave";

    client_evlp.stop_loop();
    sub_thr.join();
}

INSTANTIATE_TEST_SUITE_P(CppevTest, TestEventLoop,
                         testing::Values(fd_event_mode::level_trigger,
                                         fd_event_mode::edge_trigger,
                                         fd_event_mode::oneshot));

}  // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
