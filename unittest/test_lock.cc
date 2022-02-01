#include <gtest/gtest.h>
#include "cppev/lock.h"

namespace cppev
{

class TestLock
: public testing::Test
{
protected:
    void SetUp() override
    {
        ready_ = false;
        ready_exit_ = false;
    }

    void TearDown() override
    {}

    std::mutex lock_;

    std::condition_variable cond_;

    std::condition_variable cond_exit_;

    bool ready_;

    bool ready_exit_;
};

TEST_F(TestLock, test_rwlock_rdlocked)
{
    rwlock rwlck;
    auto func = [this, &rwlck]() -> void
    {
        std::unique_lock<std::mutex> lock(this->lock_);
        this->ready_ = true;
        rwlck.rdlock();
        this->cond_.notify_one();
        this->cond_exit_.wait(lock,
            [this]() -> bool
            {
                return this->ready_exit_;
            }
        );
        rwlck.unlock();
    };

    std::thread thr(func);

    {
        std::unique_lock<std::mutex> lock(lock_);
        if (!ready_)
        {
            cond_.wait(lock,
                [this] () -> bool
                {
                    return this->ready_;
                }
            );
        }

        ASSERT_TRUE(rwlck.try_rdlock());
        rwlck.unlock();
        ASSERT_FALSE(rwlck.try_wrlock());

        ready_exit_ = true;
        cond_exit_.notify_one();
    }
    thr.join();
}

TEST_F(TestLock, test_rwlock_wrlocked)
{
    rwlock rwlck;
    auto func = [this, &rwlck]() -> void
    {
        std::unique_lock<std::mutex> lock(this->lock_);
        this->ready_ = true;
        rwlck.wrlock();
        this->cond_.notify_one();
        this->cond_exit_.wait(lock,
            [this]() -> bool
            {
                return this->ready_exit_;
            }
        );
        rwlck.unlock();
    };

    std::thread thr(func);

    {
        std::unique_lock<std::mutex> lock(lock_);
        if (!ready_)
        {
            cond_.wait(lock,
                [this] () -> bool
                {
                    return this->ready_;
                }
            );
        }
        ASSERT_FALSE(rwlck.try_rdlock());
        ASSERT_FALSE(rwlck.try_wrlock());
        ready_exit_ = true;
        cond_exit_.notify_one();
    }
    thr.join();
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

