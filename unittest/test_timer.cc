#include <cstddef>
#include <ratio>
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
        discrete_count_helper = 0;
        total_time_ms = 200;
        freq = 500;
    }

    int count;

    int discrete_count;

    int discrete_count_helper;

    int total_time_ms;

    int freq;

    double err_percent = 0.05;

    timed_handler task = [this](const std::chrono::nanoseconds &)
    {
        ++this->count;
    };

    discrete_handler discrete_task = [this](const std::chrono::nanoseconds &,
        const std::chrono::nanoseconds &) -> bool
    {
        ++this->discrete_count;
        ++this->discrete_count_helper;
        if (discrete_count_helper % 3 != 0)
        {
            return false;
        }
        this->discrete_count_helper = 0;
        return true;
    };
};


#define CHECK_ALIGNED_TRIGGER_COUNT(count, freq, total_time_ms, err_percent) \
    EXPECT_LE(count, static_cast<int>((1 + total_time_ms / 1'000.0 * freq) * (1 + err_percent))); \
    EXPECT_GE(count, static_cast<int>((1 + (total_time_ms / 1'000.0 - 1) * freq) * (1 - err_percent)))

#define CHECK_UNALIGNED_TRIGGER_COUNT(count, freq, total_time_ms, err_percent) \
    EXPECT_LE(count, static_cast<int>((1 + total_time_ms / 1'000.0 * freq) * (1 + err_percent))); \
    EXPECT_GE(count, static_cast<int>((1 + total_time_ms / 1'000.0* freq) * (1 - err_percent)))


TEST_F(TestTimer, test_timed_multitask_scheduler_single_task)
{
    {
        timed_multitask_scheduler executor(freq, task, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(total_time_ms));
    }

    CHECK_UNALIGNED_TRIGGER_COUNT(count, freq, total_time_ms, err_percent);
}

TEST_F(TestTimer, test_timed_multitask_scheduler_single_task_with_discrete_task)
{
    bool init = false;
    bool has_exit = false;
    auto init_task = [&init]()
    {
        init = true;
    };

    auto exit_task = [&init, &has_exit]()
    {
        EXPECT_TRUE(init);
        has_exit = true;
    };

    {
        timed_multitask_scheduler executor(freq, task, discrete_task, init_task,
            exit_task, 0, std::chrono::nanoseconds(1), false);
        std::this_thread::sleep_for(std::chrono::milliseconds(total_time_ms));
    }

    EXPECT_TRUE(init);
    EXPECT_TRUE(has_exit);

    CHECK_UNALIGNED_TRIGGER_COUNT(count, freq, total_time_ms, err_percent);

    CHECK_UNALIGNED_TRIGGER_COUNT(discrete_count / 3, freq, total_time_ms, err_percent);
}

bool is_sub_sequence(const std::vector<int> &s, const std::vector<int> &t)
{
    size_t p = 0;
    size_t r = 0;
    assert(!t.empty());
    if (s.size() == 0)
    {
        return true;
    }
    while (p != s.size() && r != t.size())
    {
        if (s[p] == t[r])
        {
            ++p;
        }
        ++r;
    }
    return p == s.size();
}

TEST_F(TestTimer, test_timed_multitask_scheduler_several_timed_task)
{
    std::map<int64_t, std::vector<int>> kvmap;
    int count1 = 0;
    int count2 = 0;
    int count3 = 0;
    int count4 = 0;

    auto task1 = [&](const std::chrono::nanoseconds &stamp)
    {
        ++count1;
        kvmap[stamp.count()].push_back(1);
    };
    auto task2 = [&](const std::chrono::nanoseconds &stamp)
    {
        ++count2;
        kvmap[stamp.count()].push_back(2);
    };
    auto task3 = [&](const std::chrono::nanoseconds &stamp)
    {
        ++count3;
        kvmap[stamp.count()].push_back(3);
    };
    auto task4 = [&](const std::chrono::nanoseconds &stamp)
    {
        ++count4;
        kvmap[stamp.count()].push_back(4);
    };

    total_time_ms = 3000;
    double freq1 = 50;
    double freq2 = 20;
    double freq3 = 0.5;
    double freq4 = 40;

    {
        timed_multitask_scheduler executor(
            {
                { freq1, priority::p6, task1 },
                { freq2, priority::p0, task2 },
                { freq3, priority::p4, task3 },
                { freq4, priority::p3, task4 },
            },
            {
                { priority::p0, discrete_task },
                { priority::p0, discrete_task },
            }
        );

        std::this_thread::sleep_for(std::chrono::milliseconds(total_time_ms));
    }

    CHECK_ALIGNED_TRIGGER_COUNT(count1, freq1, total_time_ms, err_percent);

    CHECK_ALIGNED_TRIGGER_COUNT(count2, freq2, total_time_ms, err_percent);

    CHECK_ALIGNED_TRIGGER_COUNT(count3, freq3, total_time_ms, err_percent);

    CHECK_ALIGNED_TRIGGER_COUNT(count4, freq4, total_time_ms, err_percent);

    std::vector<int> res{ 2, 4, 3, 1 };

    for (auto iter = kvmap.cbegin(); iter != kvmap.cend(); ++iter)
    {
        EXPECT_TRUE(is_sub_sequence(iter->second, res));
    }
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
