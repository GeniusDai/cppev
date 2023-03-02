#include <gtest/gtest.h>
#include "cppev/timer.h"

namespace cppev
{

TEST(TestTimer, test_timed_task_executor)
{
    int count = 0;
    timer_handler handler = [&count](const std::chrono::nanoseconds &)
    {
        count++;
    };

    int timer_interval = 500;
    int total_time = 200 * 1000;
    double err_percent = 0.05;

    {
        timed_task_executor tim(std::chrono::microseconds(timer_interval), handler, false);
        std::this_thread::sleep_for(std::chrono::microseconds(total_time));
    }

    EXPECT_LE(count, (int)(total_time / timer_interval * (1 + err_percent)));
    EXPECT_GE(count, (int)(total_time / timer_interval * (1 - err_percent)));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST(TestTimer, test_timed_multitask_executor)
{
    auto task1 = [](const std::chrono::nanoseconds &stamp)
    {
        std::cout << "task 1 : " << stamp.count() << std::endl;
    };
    auto task2 = [](const std::chrono::nanoseconds &stamp)
    {
        std::cout << "task 2 : " << stamp.count() << std::endl;
    };
    auto task3 = [](const std::chrono::nanoseconds &stamp)
    {
        std::cout << "task 3 : " << stamp.count() << std::endl;
    };

    timed_multitask_executor executor({
        { 5, priority::p6, task1 },
        { 2, priority::p1, task2 },
        { 0.5, priority::p1, task3 },
    }, {});

    std::this_thread::sleep_for(std::chrono::seconds(3));
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
