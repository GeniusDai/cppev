#include <gtest/gtest.h>
#include "cppev/timer.h"

namespace cppev
{

TEST(TestTimer, test_timer)
{
    int count = 0;
    std::vector<tid> tids;
    timer::timer_handler handler = [&count, &tids]()
    {
        tids.push_back(cppev::utils::gettid());
        count++;
    };

    int timer_interval = 200;
    int total_time = 200 * 1000;
    double err_percent = 0.05;

    timer tim(std::chrono::microseconds(timer_interval), handler);
    std::this_thread::sleep_for(std::chrono::microseconds(total_time));
    tim.stop();

    EXPECT_LE(count, (int)(total_time / timer_interval * (1 + err_percent)));
    EXPECT_GE(count, (int)(total_time / timer_interval * (1 - err_percent)));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::cout << "main-tid : " << utils::gettid() << std::endl;
    std::cout << "sub-tid : " << std::endl;
    for (auto t : tids)
    {
        std::cout << t << "\t";
    }
    std::cout << std::endl;
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
