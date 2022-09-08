#include <gtest/gtest.h>
#include "cppev/linux_specific.h"

#ifdef __linux__

namespace cppev
{

TEST(TestLinuxSpecific, test_spinlock)
{
    bool ready = false;
    bool ready_exit = false;
    std::mutex lock;
    std::condition_variable cond;
    std::condition_variable cond_exit;

    spinlock splck;
    splck.lock();
    splck.unlock();
    auto func = [&]() -> void
    {
        std::unique_lock<std::mutex> lk(lock);
        ready = true;
        ASSERT_TRUE(splck.trylock());
        cond.notify_one();
        cond_exit.wait(lk,
            [&]() -> bool
            {
                return ready_exit;
            }
        );
        splck.unlock();
    };

    std::thread thr(func);

    {
        std::unique_lock<std::mutex> lk(lock);
        if (!ready)
        {
            cond.wait(lk,
                [&] () -> bool
                {
                    return ready;
                }
            );
        }
        ASSERT_FALSE(splck.trylock());
        ready_exit = true;
        cond_exit.notify_one();
    }
    thr.join();
}

TEST(TestLinuxSpecific, test_timer)
{
    int count = 0;
    timer::timer_handler handler = [&count]()
    {
        std::cout << gettid() << std::endl;
        count++;
    };
    std::cout << "main-tid : " << gettid() << std::endl;
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

#endif  // __linux__

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
