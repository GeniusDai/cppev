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

TEST_F(TestLock, test_cond_timedwait_timeout)
{
    pshared_lock lock;
    pshared_cond cond;
    cond.notify_one();
    cond.notify_all();
    std::unique_lock<pshared_lock> lk(lock);
    EXPECT_FALSE(cond.timedwait(lk, std::chrono::milliseconds(1)));
}

TEST_F(TestLock, test_cond_timedwait_in_time)
{
    pshared_lock lock;
    pshared_cond cond;

    auto func = [&]() -> void
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cond.notify_one();
    };
    std::thread thr(func);

    {
        std::unique_lock<pshared_lock> lk(lock);
        // Process sometimes receives SIGABRT if wait time is short just like 200ms
        EXPECT_TRUE(cond.timedwait(lk, std::chrono::milliseconds(2000)));
    }

    thr.join();
}

TEST_F(TestLock, test_one_time_fence_wait_first)
{
    pshared_one_time_fence one_time_fence;
    auto func = [&]() -> void
    {
        one_time_fence.wait();
        one_time_fence.wait();
        one_time_fence.wait();
    };

    std::thread thr(func);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    one_time_fence.notify();
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

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    shall_throw = false;
    EXPECT_NO_THROW(barrier.wait());

    for (int i = 0; i < num; ++i)
    {
        thrs[i].join();
    }

    EXPECT_THROW(barrier.wait(), std::logic_error);
}

template <typename Mutex>
void performance_test(Mutex &lock)
{
    int count = 0;

    int add_num = 50000;
    int thr_num = 50;

    auto task = [&]()
    {
        for (int i = 0; i < add_num; ++i)
        {
            std::unique_lock<Mutex> _(lock);
            ++count;
        }
    };

    std::vector<std::thread> thrs;
    for (int i = 0; i < thr_num; ++i)
    {
        thrs.emplace_back(task);
    }
    for (int i = 0; i < thr_num; ++i)
    {
        thrs[i].join();
    }

    EXPECT_EQ(count, add_num * thr_num);
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

TEST_F(TestLock, test_pshared_lock_performance)
{
    pshared_lock plock;
    performance_test<pshared_lock>(plock);
}

TEST_F(TestLock, test_pshared_lock_shm_performance)
{
    std::string shm_name = "/cppev_test_lock_shm";
    shared_memory shm(shm_name, sizeof(pshared_lock));
    pshared_lock *lock_ptr = shm.construct<pshared_lock>();
    performance_test<pshared_lock>(*lock_ptr);
    lock_ptr->~pshared_lock();
    shm.unlink();
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
