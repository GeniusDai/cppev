#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <ratio>
#include <unordered_map>

#include "cppev/scheduler.h"

namespace cppev
{

class TestTimedScheduler : public testing::Test
{
protected:
    void SetUp() override
    {
    }

    double err_percent = 0.1;
};

#define CHECK_UNALIGNED_TRIGGER_COUNT(count, freq, total_time_ms, err_percent) \
    EXPECT_LE(count, static_cast<int>((1 + total_time_ms / 1'000.0 * freq) *   \
                                      (1 + err_percent)));                     \
    EXPECT_GE(count, static_cast<int>((1 + total_time_ms / 1'000.0 * freq) *   \
                                      (1 - err_percent)))

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

TEST_F(TestTimedScheduler, test_timed_scheduler_several_timed_task)
{
    std::map<int64_t, std::vector<int>> kvmap;

    int total_time_ms = 3000;

    double freq1 = 50;
    double freq2 = 20;
    double freq3 = 0.5;
    double freq4 = 40;

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

    {
        timed_scheduler executor(
            {
                {freq1, priority::p6, task1},
                {freq2, priority::p0, task2},
                {freq3, priority::p4, task3},
                {freq4, priority::p3, task4},
            },
            {}, {}, false);

        std::this_thread::sleep_for(std::chrono::milliseconds(total_time_ms));
    }

    CHECK_UNALIGNED_TRIGGER_COUNT(count1, freq1, total_time_ms, err_percent);

    CHECK_UNALIGNED_TRIGGER_COUNT(count2, freq2, total_time_ms, err_percent);

    CHECK_UNALIGNED_TRIGGER_COUNT(count3, freq3, total_time_ms, err_percent);

    CHECK_UNALIGNED_TRIGGER_COUNT(count4, freq4, total_time_ms, err_percent);

    std::vector<int> res{2, 4, 3, 1};

    for (auto iter = kvmap.cbegin(); iter != kvmap.cend(); ++iter)
    {
        EXPECT_TRUE(is_sub_sequence(iter->second, res));
    }
}

}  // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
