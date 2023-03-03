#include <unordered_map>
#include <chrono>
#include <gtest/gtest.h>
#include "cppev/timer.h"

namespace cppev
{

class TestTimer
: public testing::Test
{
protected:
    void SetUp() override
    {
        count = 0;
        discrete_count = 0;
    }

    int count;

    int discrete_count;

    int freq = 500;

    int discrete_count_per_trigger = 100;

    int total_time_ms = 200;

    int total_time_ms_longer = 500;

    double err_percent = 0.05;

    timer_handler task = [this](const std::chrono::nanoseconds &)
    {
        ++this->count;
    };

    discrete_handler discrete_task = [this](const std::chrono::nanoseconds &,
        const std::chrono::nanoseconds &)
    {
        for (int i = 0; i < discrete_count_per_trigger; ++i)
        {
            ++this->discrete_count;
        }
    };
};


TEST_F(TestTimer, test_timed_task_executor)
{
    {
        timed_task_executor executor(freq, task, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(total_time_ms));
    }

    EXPECT_LE(count, (int)(total_time_ms * freq / 1'000 * (1 + err_percent)));
    EXPECT_GE(count, (int)(total_time_ms * freq / 1'000 * (1 - err_percent)));
}


TEST_F(TestTimer, test_timed_multitask_executor_single_task)
{
    {
        timed_multitask_executor executor(freq, task, {}, 0, std::chrono::nanoseconds(1), false);
        std::this_thread::sleep_for(std::chrono::milliseconds(total_time_ms));
    }

    EXPECT_LE(count, (int)(total_time_ms * freq / 1'000 * (1 + err_percent)));
    EXPECT_GE(count, (int)(total_time_ms * freq / 1'000 * (1 - err_percent)));
}


TEST_F(TestTimer, test_timed_multitask_executor_single_task_with_discrete_task)
{
    {
        timed_multitask_executor executor(freq, task, { { priority::p0, discrete_task}},
            0, std::chrono::nanoseconds(1), false);
        std::this_thread::sleep_for(std::chrono::milliseconds(total_time_ms));
    }

    EXPECT_LE(count, (int)(total_time_ms * freq / 1'000 * (1 + err_percent)));
    EXPECT_GE(count, (int)(total_time_ms * freq / 1'000 * (1 - err_percent)));

    EXPECT_LE(discrete_count, (int)(total_time_ms * freq / 1'000 * (1 + err_percent))
        * discrete_count_per_trigger);
    EXPECT_GE(discrete_count, (int)(total_time_ms * freq / 1'000 * (1 - err_percent))
        * discrete_count_per_trigger);
}


TEST_F(TestTimer, test_timed_multitask_executor)
{
    std::map<int64_t, std::vector<int>> kvmap;
    int count1 = 0;
    int count2 = 0;
    int count3 = 0;

    auto task1 = [&](const std::chrono::nanoseconds &stamp)
    {
        ++count1;
        kvmap[stamp.count()].push_back(1);
        std::cout << "task 1 : " << stamp.count() << std::endl;
    };
    auto task2 = [&](const std::chrono::nanoseconds &stamp)
    {
        ++count2;
        kvmap[stamp.count()].push_back(2);
        std::cout << "task 2 : " << stamp.count() << std::endl;
    };
    auto task3 = [&](const std::chrono::nanoseconds &stamp)
    {
        ++count3;
        kvmap[stamp.count()].push_back(3);
        std::cout << "task 3 : " << stamp.count() << std::endl;
    };

    int total_time_sec = 3;
    int freq1 = 50;

    {
        timed_multitask_executor executor({
            { freq1, priority::p6, task1 },
            { 20, priority::p0, task2 },
            { 0.5, priority::p1, task3 },
        }, {});

        std::this_thread::sleep_for(std::chrono::seconds(total_time_sec));
    }

    EXPECT_LE(count1, (int)(total_time_sec * freq1 * (1 + err_percent)));
    EXPECT_GE(count1, (int)((total_time_sec - 1) * freq1 * (1 - err_percent)));

    for (auto iter = kvmap.cbegin(); iter != kvmap.cend(); ++iter)
    {
        if (iter->second.size() == 3)
        {
            std::vector<int> res{ 2, 3, 1 };
            for (size_t i = 0; i < iter->second.size(); ++i)
            {
                EXPECT_EQ(iter->second[i], res[i]);
            }
        }
    }

}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
