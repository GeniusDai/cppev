#include <mutex>
#include <condition_variable>
#include <gtest/gtest.h>
#include "cppev/lock.h"
#include "config.h"

namespace cppev
{

class TestLock
: public testing::Test
{
protected:
    void SetUp() override
    {
        ready_ = false;
    }

    std::mutex lock_;

    std::condition_variable cond_;

    bool ready_;
};


TEST_F(TestLock, test_spinlock_performance)
{
    spinlock splck;
    performance_test<spinlock>(splck);
}

TEST_F(TestLock, test_mutex_performance)
{
    std::mutex lock;
    performance_test<std::mutex>(lock);
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
