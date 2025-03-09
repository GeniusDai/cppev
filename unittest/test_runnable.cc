#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "cppev/runnable.h"
#include "cppev/utils.h"

namespace cppev
{

const int delay = 200;

class runnable_tester : public runnable
{
public:
    void run_impl() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
};

const int sig = SIGTERM;

class runnable_tester_wait_for_signal : public runnable
{
public:
    void run_impl() override
    {
        handle_signal(sig);
        thread_wait_for_signal(sig);
    }
};

TEST(TestRunnable, test_join)
{
    runnable_tester tester;
    tester.run();
    tester.join();
}

TEST(TestRunnable, test_detach)
{
    runnable_tester tester;
    tester.run();
    tester.detach();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay * 2));
}

TEST(TestRunnable, test_wait_for_timeout)
{
    runnable_tester tester;
    tester.run();
    bool ok = tester.wait_for(std::chrono::milliseconds(delay / 10));
    EXPECT_FALSE(ok);
    tester.join();
}

TEST(TestRunnable, test_wait_for_ok)
{
    runnable_tester tester;
    tester.run();
    bool ok = tester.wait_for(std::chrono::milliseconds(delay * 2));
    EXPECT_TRUE(ok);
    tester.join();
}

TEST(TestRunnable, test_send_signal)
{
    runnable_tester_wait_for_signal tester;
    tester.run();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay / 10));
    tester.send_signal(sig);
    tester.join();
}

}  // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
