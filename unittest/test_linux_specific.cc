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
    timer::timer_handler func = [](void *ptr) -> void
    {
        std::cout << "sub-tid: " << gettid() << std::endl;
        int &val = *reinterpret_cast<int *>(ptr);
        val++;
    };
    std::cout << "main-tid : " << gettid() << std::endl;
    int timer_interval = 1;
    int total_time = 200;
    double accurate = 0.1;
    timer tim(1, func, &count);
    std::this_thread::sleep_for(std::chrono::milliseconds(total_time));
    tim.stop();
    std::this_thread::sleep_for(std::chrono::microseconds(total_time));

    EXPECT_LE(count, (int)(total_time / timer_interval * (1 + accurate)));
    EXPECT_GE(count, (int)(total_time / timer_interval * (1 - accurate)));
}

}   // namespace cppev

#endif  // __linux__

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
