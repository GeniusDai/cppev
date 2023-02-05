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
    {
    }

    std::mutex lock_;

    std::condition_variable cond_;

    bool ready_;
};

TEST_F(TestLock, test_rwlock_guard_movable)
{
    pshared_rwlock rwlck;

    {
        rdlockguard lg(rwlck);
        rdlockguard lg1(std::move(lg));
        lg = std::move(lg1);
    }
    EXPECT_TRUE(rwlck.try_rdlock());
    rwlck.unlock();

    {
        wrlockguard lg(rwlck);
        wrlockguard lg1(std::move(lg));
        lg = std::move(lg1);
    }
    EXPECT_TRUE(rwlck.try_wrlock());
    rwlck.unlock();
}

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

TEST_F(TestLock, test_cond_wait_timeout)
{
    pshared_lock lock;
    pshared_cond cond;
    std::unique_lock<pshared_lock> lk(lock);
    EXPECT_FALSE(cond.timedwait(lk, std::chrono::milliseconds(1)));
}

TEST_F(TestLock, test_cond_wait_in_time)
{
    pshared_lock lock;
    pshared_cond cond;

    auto func = [&]() -> void
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        cond.notify_one();
    };
    std::thread thr(func);

    std::unique_lock<pshared_lock> lk(lock);
    EXPECT_TRUE(cond.timedwait(lk, std::chrono::milliseconds(20)));

    thr.join();
}

TEST_F(TestLock, test_one_time_fence_wait_first)
{
    pshared_one_time_fence one_time_fence;
    auto func = [&]() -> void
    {
        one_time_fence.wait();
        one_time_fence.wait();
    };

    std::thread thr(func);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    one_time_fence.notify();
    thr.join();
}

TEST_F(TestLock, test_one_time_fence_notify_first)
{
    pshared_one_time_fence one_time_fence;
    auto func = [&]() -> void
    {
        one_time_fence.wait();
        one_time_fence.wait();
    };

    one_time_fence.notify();
    std::thread thr(func);
    thr.join();
}

TEST_F(TestLock, test_barrier_throw)
{
    pshared_barrier barrier(1);
    EXPECT_NO_THROW(barrier.wait());
    EXPECT_THROW(barrier.wait(), std::logic_error);
}

TEST_F(TestLock, test_barrier_multithread)
{
    const int num = 10;
    pshared_barrier barrier(num + 1);
    std::vector<std::thread> thrs;
    bool shall_throw = true;

    auto func = [&]() -> void
    {
        barrier.wait();
        if (shall_throw)
        {
            throw_runtime_error("test not ok!");
        }
    };

    for (int i = 0; i < num; ++i)
    {
        thrs.push_back(std::thread(func));
    }

    shall_throw = false;
    barrier.wait();

    for (int i = 0; i < num; ++i)
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
