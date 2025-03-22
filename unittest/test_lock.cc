#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>

#include "config.h"
#include "cppev/lock.h"

namespace cppev
{

class TestLockByThread : public testing::TestWithParam<sync_level>
{
protected:
    void SetUp() override
    {
        ready_ = false;
    }

    mutex lock_{sync_level::thread};

    cond cond_{sync_level::thread};

    bool ready_;
};

TEST_P(TestLockByThread, test_rwlock_guard_movable)
{
    rwlock rwlck(GetParam());

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

TEST_P(TestLockByThread, test_rwlock_rdlocked)
{
    rwlock rwlck(GetParam());

    // sub-thread
    auto func = [this, &rwlck]()
    {
        std::unique_lock<mutex> lock(this->lock_);
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
        std::unique_lock<mutex> lock(lock_);
        if (!ready_)
        {
            cond_.wait(lock, [this]() -> bool { return this->ready_; });
        }

        ASSERT_TRUE(rwlck.try_rdlock());
        rwlck.unlock();
        ASSERT_FALSE(rwlck.try_wrlock());
        cond_.notify_one();
    }
    thr.join();
}

TEST_P(TestLockByThread, test_rwlock_wrlocked)
{
    rwlock rwlck(GetParam());

    // sub-thread
    auto func = [this, &rwlck]()
    {
        std::unique_lock<mutex> lock(this->lock_);
        this->ready_ = true;
        rwlck.wrlock();
        this->cond_.notify_one();
        this->cond_.wait(lock);
        rwlck.unlock();
    };
    std::thread thr(func);

    // main-thread
    {
        std::unique_lock<mutex> lock(lock_);
        if (!ready_)
        {
            cond_.wait(lock, [this]() -> bool { return this->ready_; });
        }
        ASSERT_FALSE(rwlck.try_rdlock());
        ASSERT_FALSE(rwlck.try_wrlock());
        cond_.notify_one();
    }
    thr.join();
}

TEST_P(TestLockByThread, test_one_time_fence_wait_first)
{
    one_time_fence otf(GetParam());
    auto func = [&]() -> void
    {
        otf.wait();
        otf.wait();
        otf.wait();
    };

    std::thread thr(func);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    otf.notify();
    otf.notify();
    thr.join();
}

TEST_P(TestLockByThread, test_one_time_fence_notify_first)
{
    one_time_fence otf(GetParam());
    auto func = [&]() -> void
    {
        otf.wait();
        otf.wait();
    };

    otf.notify();
    std::thread thr(func);
    thr.join();
}

TEST_P(TestLockByThread, test_barrier_throw)
{
    barrier br(GetParam(), 1);
    EXPECT_NO_THROW(br.wait());
    EXPECT_THROW(br.wait(), std::logic_error);
}

TEST_P(TestLockByThread, test_barrier_multithread)
{
    const int num = 10;
    barrier br(GetParam(), num + 1);
    std::vector<std::thread> thrs;
    bool shall_throw = true;

    auto func = [&]() -> void
    {
        br.wait();
        if (shall_throw)
        {
            throw_runtime_error("test not ok!");
        }
    };

    for (int i = 0; i < num; ++i)
    {
        thrs.push_back(std::thread(func));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    shall_throw = false;
    EXPECT_NO_THROW(br.wait());

    for (int i = 0; i < num; ++i)
    {
        thrs[i].join();
    }

    EXPECT_THROW(br.wait(), std::logic_error);
}

TEST_P(TestLockByThread, test_mutex_performance)
{
    mutex plock(GetParam());
    performance_test<mutex>(plock);
}

INSTANTIATE_TEST_SUITE_P(CppevTest, TestLockByThread,
                         testing::Values(sync_level::thread,
                                         sync_level::process));

}  // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
