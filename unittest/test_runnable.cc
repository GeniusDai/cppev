#include <thread>
#include <chrono>
#include <gtest/gtest.h>
#include "cppev/runnable.h"

namespace cppev
{

int status = 66;
int delay = 50;

class runnable_tester
: public runnable
{
public:
    void run_impl() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
};

TEST(TestRunnable, test_cancel)
{
    runnable_tester tester;
    tester.run();
    tester.cancel();
    tester.join();
}

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
    bool ok = tester.wait_for(std::chrono::milliseconds(1));
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

}

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
