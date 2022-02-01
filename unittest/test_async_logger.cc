#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <random>
#include "cppev/sysconfig.h"
#include "cppev/async_logger.h"

namespace cppev
{

const char *str = "Cppev is a C++ event driven library";

class TestAsyncLogger
: public testing::Test
{
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}

    int thr_num = 50;

    int loop_num = 100;
};

TEST_F(TestAsyncLogger, test_output)
{
    sysconfig::buffer_outdate = 1;
    auto output = [this](int i) -> void
    {
        for (int j = 0; j < this->loop_num; ++j)
        {
            log::info << str << " round [" << i << "]" << log::endl;
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            log::error << str << " round [" << i << "]" << log::endl;
        }
        std::random_device rd;
        std::default_random_engine rde(rd());
        std::uniform_int_distribution<int> dist(0, 1500);
        int value = dist(rde);
        std::this_thread::sleep_for(std::chrono::milliseconds(value));
        float x1 = value;
        double x2 = value;
        int x3 = value;
        long x4 = value;
        for (int j = 0; j < this->loop_num; ++j)
        {
            log::info
                << "Slept for "
                << x1 << " "
                << x2 << " "
                << x3 << " "
                << x4 << " "
            << log::endl;
            log::error
                << "Slept for "
                << x1 << " "
                << x2 << " "
                << x3 << " "
                << x4 << " "
            << log::endl;
        }
    };
    std::vector<std::thread> thrs;
    for (int i = 0; i < thr_num; ++i)
    {
        thrs.emplace_back(output, i);
    }
    for (int i = 0; i < thr_num; ++i)
    {
        thrs[i].join();
    }
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}