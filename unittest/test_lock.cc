#include <mutex>
#include <condition_variable>
#include <gtest/gtest.h>
#include "cppev/lock.h"
#include "cppev/ipc.h"

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

    void TearDown() override
    {}

    std::mutex lock_;

    std::condition_variable cond_;

    bool ready_;
};

TEST_F(TestLock, test_rwlock_rdlocked)
{
    pshared_rwlock rwlck;

    // sub-thread
    auto func = [this, &rwlck]()
    {
        std::unique_lock<std::mutex> lock(this->lock_);
        this->ready_ = true;
        rwlck.rdlock();
        this->cond_.notify_one();
        this->cond_.wait(lock);
        rwlck.unlock();
        ASSERT_TRUE(rwlck.try_wrlock());
        rwlck.unlock();
    };
    std::thread thr(func);

    // main-thread
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
        cond_.notify_one();
    }
    thr.join();
}

TEST_F(TestLock, test_rwlock_wrlocked)
{
    pshared_rwlock rwlck;

    // sub-thread
    auto func = [this, &rwlck]()
    {
        std::unique_lock<std::mutex> lock(this->lock_);
        this->ready_ = true;
        rwlck.wrlock();
        this->cond_.notify_one();
        this->cond_.wait(lock);
        rwlck.unlock();
    };
    std::thread thr(func);

    // main-thread
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
        cond_.notify_one();
    }
    thr.join();
}

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
            cond_.wait(lk,
                [this] () -> bool
                {
                    return this->ready_;
                }
            );
        }
        ASSERT_FALSE(splck.trylock());
        cond_.notify_one();
    }
    thr.join();
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
