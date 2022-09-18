#include <gtest/gtest.h>
#include "cppev/timer.h"

namespace cppev
{

TEST(TestTimer, test_timer)
{
    int count = 0;
    timer::timer_handler handler = [&count]()
    {
        std::cout << utils::gettid() << std::endl;
        count++;
    };
    std::cout << "main-tid : " << utils::gettid() << std::endl;
    std::cout << "sub-tid : " << std::endl;
    int timer_interval = 500;
    int total_time = 200 * 1000;
    double err_percent = 0.10;

    timer tim(std::chrono::microseconds(timer_interval), handler);
    std::this_thread::sleep_for(std::chrono::microseconds(total_time));
    tim.stop();

    EXPECT_LE(count, (int)(total_time / timer_interval * (1 + err_percent)));
    EXPECT_GE(count, (int)(total_time / timer_interval * (1 - err_percent)));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
