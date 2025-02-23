#include <unordered_set>
#include <fcntl.h>
#include <gtest/gtest.h>
#include "cppev/nio.h"
#include "cppev/event_loop.h"

namespace cppev
{

TEST(TestEvlp, test_enum_class_operator_or)
{
    fd_event fd1 = fd_event::fd_readable;
    fd_event fd2 = fd_event::fd_writable;
    fd_event fd3 = fd1 | fd2;
    fd2 |= fd1;
    ASSERT_EQ(fd3, fd2);
    ASSERT_NE(fd3, fd1);
}

TEST(TestEvlp, test_enum_class_operator_and)
{
    fd_event fd1 = fd_event::fd_readable;
    fd_event fd2 = fd_event::fd_writable;
    fd_event fd3 = fd1 & fd2;
    fd2 &= fd1;
    ASSERT_EQ(fd3, fd2);
}

TEST(TestEvlp, test_enum_class_operator_xor)
{
    fd_event fd1 = fd_event::fd_readable;
    fd_event fd2 = fd_event::fd_writable;
    fd_event fd3 = fd1 ^ fd2;
    fd_event fd4 = fd1 | fd2;
    fd2 ^= fd1;
    ASSERT_EQ(fd3, fd4);
    ASSERT_EQ(fd3, fd2);
    fd2 ^= fd1;
    ASSERT_EQ(fd1^fd2, fd4);
    ASSERT_EQ(fd4^fd2, fd1);
    ASSERT_EQ(fd4^fd1, fd2);
}

const char *str = "Cppev is a C++ event driven library";

TEST(TestEvlp, test_tcp_connect_with_evlp)
{
    std::vector<std::tuple<family, int, std::string>> vec =
    {
        { family::ipv4, 8884, "127.0.0.1" },
        { family::ipv6, 8886, "::1"       },
    };

    int acpt_count = 0;
    event_loop acpt_evlp(&acpt_count);

    int cont_count = 0;
    event_loop cont_evlp(&cont_count);

    fd_event_handler acpt_callback = [&](const std::shared_ptr<nio> &iop)
    {
        (*reinterpret_cast<int *>(iop->evlp().data()))++;
        auto conns = std::dynamic_pointer_cast<nsocktcp>(iop)->accept();
        for (auto conn : conns)
        {
            acpt_evlp.fd_register_and_activate(
                std::static_pointer_cast<nio>(conn),
                fd_event::fd_writable,
                [](const std::shared_ptr<nio> &iop) {
                    std::cout << "writeable event for connected socket triggered" << std::endl;
                    auto iopt = std::dynamic_pointer_cast<nsocktcp>(iop);
                    iopt->wbuffer().put_string(str);
                    iopt->write_all();
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            );
        }
    };

    fd_event_handler cont_callback = [&](const std::shared_ptr<nio> &iop)
    {
        (*reinterpret_cast<int *>(iop->evlp().data()))++;
    };

    for (size_t i = 0; i < vec.size(); ++i)
    {
        // Test Event Loop API
        auto listensock = nio_factory::get_nsocktcp(std::get<0>(vec[i]));
        listensock->bind(std::get<1>(vec[i]));
        listensock->listen();
        auto acpt_niop = std::dynamic_pointer_cast<nio>(listensock);
        acpt_evlp.fd_register_and_activate(acpt_niop, fd_event::fd_readable, acpt_callback);
        acpt_evlp.fd_remove_and_deactivate_all(acpt_niop);
        acpt_evlp.fd_register_and_activate(acpt_niop, fd_event::fd_readable, acpt_callback);
    }

    std::thread thr_cont([&]() {
        for (size_t i = 0; i < vec.size(); ++i)
        {
            auto connsock = nio_factory::get_nsocktcp(std::get<0>(vec[i]));
            EXPECT_TRUE(connsock->connect(std::get<2>(vec[i]), std::get<1>(vec[i])));
            auto conn_niop = std::dynamic_pointer_cast<nio>(connsock);
            cont_evlp.fd_register(conn_niop, fd_event::fd_writable, cont_callback);
            cont_evlp.fd_activate(conn_niop, fd_event::fd_writable);
            cont_evlp.fd_deactivate(conn_niop, fd_event::fd_writable);
            cont_evlp.fd_activate(conn_niop, fd_event::fd_writable);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cont_evlp.loop_once();
    });
    thr_cont.join();

    std::thread thr_stop([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        acpt_evlp.stop_loop_forever();
    });

    acpt_evlp.loop_forever();
    thr_stop.join();

    EXPECT_EQ(acpt_count, 2);
    EXPECT_EQ(cont_count, 2);

    std::thread thr1([&]() {
        acpt_evlp.loop_once();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    acpt_evlp.stop_loop_once();
    thr1.join();
    std::cout << "loop once stopped" << std::endl;

    std::thread thr2([&]() {
        acpt_evlp.loop_forever();
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    acpt_evlp.stop_loop_forever();
    thr2.join();
    std::cout << "loop forever stopped" << std::endl;
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
