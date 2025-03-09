#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>

#include "config.h"
#include "cppev/lock.h"

namespace cppev
{

class TestLock : public testing::Test
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

TEST_F(TestLock, test_spinlock)
{
    spinlock splck;

    // sub-thread
    auto func = [this, &splck]() -> void
    {
        std::unique_lock<std::mutex> lock(lock_);
        this->ready_ = true;
        ASSERT_TRUE(splck.trylock());
        this->cond_.notify_one();
        this->cond_.wait(lock);
        splck.unlock();
    };
    std::thread thr(func);

    // main-thread
    {
        std::unique_lock<std::mutex> lk(lock_);
        if (!ready_)
        {
            cond_.wait(lk, [this]() -> bool { return this->ready_; });
        }
        ASSERT_FALSE(splck.trylock());
        cond_.notify_one();
    }
    thr.join();
}

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

}  // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
