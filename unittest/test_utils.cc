#include <unistd.h>
#include <gtest/gtest.h>
#include "cppev/utils.h"

namespace cppev
{

TEST(TestCommonUtils, test_split)
{
    std::vector<std::tuple<std::string, std::string, std::vector<std::string>>> cases =
    {
        { "hello, cppev", ",", { "hello", " cppev"} },
        { ", hello, cppev, ", ", ", {"", "hello", "cppev", ""} },
        { "hello", "invalid", { "hello" } },
        { "hellohello", "invalid", { "hellohello" } },
        { "***", "*", { "", "", "", "" } },
        { "**cppev**", "*", { "", "", "cppev", "", "" } },
    };

    for (const auto &tc : cases)
    {
        EXPECT_EQ(std::get<2>(tc), utils::split(std::get<0>(tc), std::get<1>(tc)));
    }
}

TEST(TestCommonUtils, test_join)
{
    std::vector<std::tuple<std::vector<std::string>, std::string, std::string>> cases =
    {
        { {"hi"}, "\n", "hi" },
        { {"cppev", "nice"}, "\t", "cppev\tnice" },
        { {""}, "", "" },
        { {"", ""}, "\t", "\t"},
        { {"", "cppev", "cppev", ""}, std::string("\0", 1), std::string("\0cppev\0cppev\0", 13) },
    };

    for (const auto &tc : cases)
    {
        EXPECT_EQ(std::get<2>(tc), utils::join(std::get<0>(tc), std::get<1>(tc)));
    }
}

typedef void (*testing_func_type)(int, bool);

class TestSignal
: public testing::TestWithParam<
    std::tuple<
        std::tuple<std::string, testing_func_type>,
        int,
        bool
    >>
{
};

const int delay = 10;

void test_main_thread_signal_wait(int sig, bool block)
{
    reset_signal(sig);
    if (block)
    {
        thread_block_signal(sig);
    }
    else
    {
        thread_unblock_signal(sig);
        handle_signal(sig,
            [](int sig)
            {
                std::cout << "handling signal " << sig << std::endl;
            }
        );
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
        EXPECT_FALSE(thread_check_signal_pending(sig));
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
    reset_signal(sig);
    pid_t pid = fork();
    if (pid == -1)
    {
        throw_system_error("fork error");
    }
    if (pid == 0)
    {
        std::thread thr(
            [=]()
            {
                if (block)
                {
                    thread_block_signal(sig);
                    EXPECT_TRUE(thread_check_signal_mask(sig));
                    thread_raise_signal(sig);
                    thread_raise_signal(sig);
                    EXPECT_TRUE(thread_check_signal_pending(sig));
                    thread_wait_for_signal(sig);
                    EXPECT_FALSE(thread_check_signal_pending(sig));
                }
                else
                {
                    thread_unblock_signal(sig);
                    handle_signal(sig,
                        [](int sig)
                        {
                            std::cout << "handling signal " << sig << std::endl;
                        }
                    );
                    EXPECT_FALSE(thread_check_signal_mask(sig));
                    EXPECT_FALSE(thread_check_signal_pending(sig));
                }
                thread_wait_for_signal(sig);
            }
        );

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

void test_main_thread_signal_suspend(int sig, bool block)
{
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
        thread_suspend_for_signal(sig);
        EXPECT_FALSE(thread_check_signal_pending(sig));
        _exit(0);
    }

    // waiting for subprocess waiting for signal
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    send_signal(pid, sig);

    int ret = -1;
    waitpid(pid, &ret, 0);
    EXPECT_EQ(ret, 0);
}

void test_sub_thread_signal_suspend(int sig, bool block)
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
        std::thread thr(
            [=]()
            {
                if (block)
                {
                    thread_block_signal(sig);
                    EXPECT_TRUE(thread_check_signal_mask(sig));
                    thread_raise_signal(sig);
                    thread_raise_signal(sig);
                    EXPECT_TRUE(thread_check_signal_pending(sig));
                    thread_suspend_for_signal(sig);
                    EXPECT_FALSE(thread_check_signal_pending(sig));
                }
                else
                {

                    thread_unblock_signal(sig);
                    EXPECT_FALSE(thread_check_signal_mask(sig));
                    EXPECT_FALSE(thread_check_signal_pending(sig));
                }
                thread_suspend_for_signal(sig);
            }
        );

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
    std::get<1>(std::get<0>(param))(std::get<1>(param), std::get<2>(param));
}

INSTANTIATE_TEST_SUITE_P(CppevTest, TestSignal,
    testing::Combine(
        testing::Values(
            std::tuple<std::string, testing_func_type>{
                "test_main_thread_signal_wait", test_main_thread_signal_wait },
            std::tuple<std::string, testing_func_type>{
                "test_sub_thread_signal_wait", test_sub_thread_signal_wait },
            std::tuple<std::string, testing_func_type>{
                "test_main_thread_signal_suspend", test_main_thread_signal_suspend },
            std::tuple<std::string, testing_func_type>{
                "test_sub_thread_signal_suspend", test_sub_thread_signal_suspend }
        ),
        testing::Values(SIGINT, SIGABRT, SIGPIPE, SIGTERM, SIGCONT, SIGTSTP),
        testing::Values(true, false)
    )
);

TEST(TestCheckBySignal, test_check_pid_and_pgid)
{
    pid_t pid = fork();
    if (pid == -1)
    {
        throw_system_error("fork error");
    }
    if (pid == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        EXPECT_FALSE(check_process_group(getpid()));
        _exit(0);
    }

    // waiting for subprocess waiting for signal
    EXPECT_TRUE(check_process(pid));
    EXPECT_FALSE(check_process_group(pid));
    EXPECT_THROW(check_process(0) && check_process(-1), std::logic_error);

    int ret = -1;
    waitpid(pid, &ret, 0);
    EXPECT_EQ(ret, 0);
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}