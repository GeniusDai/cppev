#include <unistd.h>
#include <gtest/gtest.h>
#include "cppev/utils.h"

namespace cppev
{

TEST(TestCommonUtils, test_split)
{
    std::vector<std::string> subs;

    subs = utils::split("hello, cppev", ",");
    EXPECT_EQ(subs.size(), 2);

    subs = utils::split("hello, cppev", ", ");
    EXPECT_EQ(subs.size(), 2);

    subs = utils::split(", hello, cppev, ", ", ");
    EXPECT_EQ(subs.size(), 4);
    EXPECT_STREQ(subs[0].c_str(), "");
    EXPECT_STREQ(subs[1].c_str(), "hello");
    EXPECT_STREQ(subs[2].c_str(), "cppev");
    EXPECT_STREQ(subs[3].c_str(), "");

    subs = utils::split(",hello,cppev,", ",");
    EXPECT_EQ(subs.size(), 4);
    EXPECT_STREQ(subs[0].c_str(), "");
    EXPECT_STREQ(subs[1].c_str(), "hello");
    EXPECT_STREQ(subs[2].c_str(), "cppev");
    EXPECT_STREQ(subs[3].c_str(), "");
}

typedef void (*testing_func_type)(int, bool);

class TestSignal
: public testing::TestWithParam<std::tuple<testing_func_type, int, bool>>
{
};

const int delay = 50;

void test_main_thread_signal_wait(int sig, bool block)
{
    // although this handler will not be executed, but you need to change
    // the handler of signal, in linux some of the signals will cause the
    // process terminate.
    handle_signal(sig,
        [](int sig)
        {
            std::cout << "handling signal " << sig << std::endl;
        }
    );
    if (block)
    {
        thread_block_signal(sig);
    }
    else
    {
        thread_unblock_signal(sig);
    }

    pid_t pid = fork();
    if (pid == -1)
    {
        throw_system_error("fork error");
    }
    if (pid == 0)
    {
        EXPECT_FALSE(thread_check_signal_pending(sig));
        thread_wait_for_signal(sig);
        _exit(0);
    }

    // waiting for subprocess waiting for signal
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    send_signal(pid, sig);

    int ret = -1;
    waitpid(pid, &ret, 0);
    EXPECT_EQ(ret, 0);
}

void test_sub_thread_signal_wait(int sig, bool block)
{
    handle_signal(sig,
        [](int sig)
        {
            std::cout << "handling signal " << sig << std::endl;
        }
    );
    pid_t pid = fork();
    if (pid == -1)
    {
        throw_system_error("fork error");
    }
    if (pid == 0)
    {
        thread_unblock_signal(sig);
        std::thread thr(
            [=]()
            {
                if (block)
                {
                    thread_block_signal(sig);
                    EXPECT_TRUE(thread_check_signal_mask(sig));
                    thread_raise_signal(sig);
                    EXPECT_TRUE(thread_check_signal_pending(sig));
                    thread_wait_for_signal(sig);
                }
                else
                {
                    thread_unblock_signal(sig);
                    EXPECT_FALSE(thread_check_signal_mask(sig));
                    EXPECT_FALSE(thread_check_signal_pending(sig));
                }
                thread_wait_for_signal(sig);
            }
        );
        EXPECT_FALSE(thread_check_signal_mask(sig));
        thread_block_signal(sig);
        EXPECT_TRUE(thread_check_signal_mask(sig));
        thr.join();
        EXPECT_FALSE(thread_check_signal_pending(sig));
        _exit(0);
    }

    // waiting for subprocess block signal
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    send_signal(pid, sig);

    int ret = -1;
    waitpid(pid, &ret, 0);
    EXPECT_EQ(ret, 0);
}

TEST_P(TestSignal, test_signal_all)
{
    auto param = GetParam();
    std::get<0>(param)(std::get<1>(param), std::get<2>(param));
}

INSTANTIATE_TEST_SUITE_P(CppevTest, TestSignal,
    testing::Combine(
        testing::Values(test_main_thread_signal_wait, test_sub_thread_signal_wait),
        testing::Values(SIGINT, SIGABRT, SIGPIPE, SIGTERM, SIGCONT, SIGTSTP),
        testing::Bool()
    )
);


}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}