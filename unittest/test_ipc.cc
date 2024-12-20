#include <thread>
#include <mutex>
#include <condition_variable>
#include <gtest/gtest.h>
#include "cppev/ipc.h"
#include "cppev/lock.h"
#include "config.h"

namespace cppev
{

class TestIpcByFork
: public testing::Test
{
protected:
    TestIpcByFork()
    : name_("/cppev_test_ipc_name")
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    std::string name_;
};

struct TestStructBase
{
    TestStructBase()
    {
        std::cout << "constructor" << std::endl;
    }

    ~TestStructBase()
    {
        std::cout << "destructor" << std::endl;
    }
};

const int delay = 50;

TEST_F(TestIpcByFork, test_sem_shm_by_fork)
{
    int shm_size = 12;

    std::vector<semaphore> sem_vec;
    sem_vec.emplace_back(name_);
    EXPECT_TRUE(sem_vec[0].creator());
    sem_vec[0].unlink();
    sem_vec.pop_back();

    std::vector<shared_memory> shm_vec;
    shm_vec.emplace_back(name_, shm_size);
    EXPECT_TRUE(shm_vec[0].creator());
    shm_vec[0].unlink();
    shm_vec.clear();

    pid_t pid = fork();
    if (pid < 0)
    {
        throw_system_error("fork error");
    }
    else if (pid == 0)
    {
        semaphore sem(name_);
        sem.acquire();

        shared_memory shm(name_, shm_size);
        EXPECT_EQ(std::string(reinterpret_cast<char *>(shm.ptr())), "cppev");
        EXPECT_FALSE(sem.try_acquire());
        sem.release(3);
        semaphore sem1(std::move(sem));
        EXPECT_TRUE(sem1.try_acquire());
        sem1.acquire(2);
        sem = std::move(sem1);
        EXPECT_FALSE(sem.try_acquire());

        std::cout << "shared memory ptr : " << shm.ptr() << std::endl;

        if (shm.creator())
        {
            std::cout << "subprocess is shm's creator" << std::endl;
            shm.unlink();
        }
        if (sem.creator())
        {
            std::cout << "subprocess is sem's creator" << std::endl;
            sem.unlink();
        }

        std::cout << "end of child process" << std::endl;
        _exit(0);
    }
    else
    {
        shared_memory shm(name_, shm_size);
        shared_memory shm1(std::move(shm));
        memcpy(shm1.ptr(), "cppev", 5);
        shm = std::move(shm1);

        semaphore sem(name_);
        sem.release();

        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);

        if (shm.creator())
        {
            std::cout << "main-process is shm's creator" << std::endl;
            shm.unlink();
        }
        if (sem.creator())
        {
            std::cout << "main-process is sem's creator" << std::endl;
            sem.unlink();
        }
    }
}

TEST_F(TestIpcByFork, test_sem_shm_rwlock_by_fork)
{
    struct TestStruct : public TestStructBase
    {
        TestStruct(int var1, double var2)
        : var1(var1), var2(var2)
        {
        }

        pshared_rwlock lock;
        int var1;
        double var2;
    };

    pid_t pid = fork();
    if (pid < 0)
    {
        throw_system_error("fork error");
    }
    else if (pid == 0)
    {
        semaphore sem(name_);
        sem.acquire();
        sem.unlink();

        shared_memory shm(name_, sizeof(TestStruct));
        TestStruct *ptr = reinterpret_cast<TestStruct *>(shm.ptr());

        EXPECT_TRUE(ptr->lock.try_rdlock());
        ptr->lock.unlock();

        ptr->~TestStruct();
        shm.unlink();

        _exit(0);
    }
    else
    {
        shared_memory shm(name_, sizeof(TestStruct));
        TestStruct *ptr = shm.construct<TestStruct, int, double>(0, 6.6);
        EXPECT_EQ(reinterpret_cast<void *>(ptr), shm.ptr());

        EXPECT_TRUE(ptr->lock.try_wrlock());
        ptr->lock.unlock();
        EXPECT_TRUE(ptr->lock.try_rdlock());
        ptr->lock.unlock();

        EXPECT_TRUE(ptr->lock.try_rdlock());
        ptr->lock.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        semaphore sem(name_);
        sem.release();

        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);
    }
}

TEST_F(TestIpcByFork, test_sem_shm_lock_cond_by_fork)
{
    struct TestStruct : public TestStructBase
    {
        TestStruct()
        : var(0), ready(false)
        {
        }

        pshared_lock lock;
        pshared_cond cond;
        int var;
        bool ready;
    };

    int NUMBER = 100;
    pid_t pid = fork();
    if (pid < 0)
    {
        throw_system_error("fork error");
    }
    else if (pid == 0)
    {
        semaphore sem(name_);
        sem.acquire();
        sem.unlink();

        shared_memory shm(name_, sizeof(TestStruct));
        TestStruct *ptr = reinterpret_cast<TestStruct *>(shm.ptr());

        {
            std::unique_lock<pshared_lock> lock(ptr->lock);
            ptr->var = NUMBER;
            ptr->ready = true;
            ptr->cond.notify_one();
            ptr->cond.wait(lock);
        }

        EXPECT_TRUE(ptr->lock.try_lock());
        ptr->lock.unlock();

        ptr->~TestStruct();
        shm.unlink();

        _exit(0);
    }
    else
    {
        shared_memory shm(name_, sizeof(TestStruct));
        TestStruct *ptr = shm.construct<TestStruct>();
        EXPECT_EQ(ptr, reinterpret_cast<TestStruct *>(shm.ptr()));

        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        semaphore sem(name_);
        sem.release();

        {
            std::unique_lock<pshared_lock> lock(ptr->lock);
            if(!ptr->ready)
            {
                ptr->cond.wait(lock);
            }

            EXPECT_EQ(ptr->var, NUMBER);

            ptr->cond.notify_one();
        }

        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);
    }

}

TEST_F(TestIpcByFork, test_shm_one_time_fence_barrier_by_fork)
{
    struct TestStruct : public TestStructBase
    {
        TestStruct()
        : barrier(2), var(0)
        {
        }

        pshared_one_time_fence one_time_fence;
        pshared_barrier barrier;
        int var;
    };

    pid_t pid = fork();
    if (pid < 0)
    {
        throw_system_error("fork error");
    }
    else if (pid == 0)
    {
        shared_memory shm(name_, sizeof(TestStruct));
        TestStruct *ptr = reinterpret_cast<TestStruct *>(shm.ptr());
        if (shm.creator())
        {
            shm.construct<TestStruct>();
        }

        ptr->one_time_fence.wait();
        EXPECT_TRUE(ptr->one_time_fence.ok());
        ptr->one_time_fence.wait();
        EXPECT_EQ(ptr->var, 100);
        ptr->barrier.wait();

        if (shm.creator())
        {
            shm.unlink();
        }
        _exit(0);
    }
    else
    {
        shared_memory shm(name_, sizeof(TestStruct));
        TestStruct *ptr = reinterpret_cast<TestStruct *>(shm.ptr());
        if (shm.creator())
        {
            shm.construct<TestStruct>();
        }

        ptr->var = 100;
        EXPECT_FALSE(ptr->one_time_fence.ok());
        ptr->one_time_fence.notify();
        EXPECT_TRUE(ptr->one_time_fence.ok());
        ptr->one_time_fence.wait();
        EXPECT_TRUE(ptr->one_time_fence.ok());

        ptr->barrier.wait();
        EXPECT_THROW(ptr->barrier.wait(), std::logic_error);

        if (shm.creator())
        {
            shm.unlink();
        }
        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);
    }
}

class TestPSharedLockByThread
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


TEST_F(TestPSharedLockByThread, test_rwlock_guard_movable)
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

TEST_F(TestPSharedLockByThread, test_rwlock_rdlocked)
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

TEST_F(TestPSharedLockByThread, test_rwlock_wrlocked)
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

TEST_F(TestPSharedLockByThread, test_spinlock)
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

TEST_F(TestPSharedLockByThread, test_one_time_fence_wait_first)
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

TEST_F(TestPSharedLockByThread, test_one_time_fence_notify_first)
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

TEST_F(TestPSharedLockByThread, test_barrier_throw)
{
    pshared_barrier barrier(1);
    EXPECT_NO_THROW(barrier.wait());
    EXPECT_THROW(barrier.wait(), std::logic_error);
}

TEST_F(TestPSharedLockByThread, test_barrier_multithread)
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

TEST_F(TestPSharedLockByThread, test_pshared_lock_performance)
{
    pshared_lock plock;
    performance_test<pshared_lock>(plock);
}

TEST_F(TestPSharedLockByThread, test_pshared_lock_shm_performance)
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
