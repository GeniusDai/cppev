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
    pshared_rwlock rwlck;
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
    pshared_rwlock rwlck;
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

#if _POSIX_C_SOURCE >= 200112L

TEST_F(TestLock, test_spinlock)
{
    spinlock splck;
    splck.lock();
    splck.unlock();
    auto func = [this, &splck]() -> void
    {
        std::unique_lock<std::mutex> lock(this->lock_);
        this->ready_ = true;
        ASSERT_TRUE(splck.trylock());
        this->cond_.notify_one();
        this->cond_exit_.wait(lock,
            [this]() -> bool
            {
                return this->ready_exit_;
            }
        );
        splck.unlock();
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
        ASSERT_FALSE(splck.trylock());
        ready_exit_ = true;
        cond_exit_.notify_one();
    }
    thr.join();
}

#endif  // spinlock

TEST_F(TestLock, test_shm_rwlock)
{
    struct TestStruct
    {
        TestStruct(int var1, double var2) : var1(var1), var2(var2)
        {
            std::cout << "constructor" << std::endl;
        }
        ~TestStruct()
        {
            std::cout << "destructor" << std::endl;
        }
        pshared_rwlock lock;
        int var1;
        double var2;
    };

    shared_memory shm("/cppev_test_shm_lock", sizeof(TestStruct), true);
    shm.constructor<TestStruct, int, double>(0, 6.6);
    TestStruct *f_ptr = reinterpret_cast<TestStruct *>(shm.ptr());
    EXPECT_TRUE(f_ptr->lock.try_wrlock());
    f_ptr->lock.unlock();

    pid_t pid = fork();
    if (pid < 0)
    {
        throw_system_error("fork error");
    }
    if (pid == 0)
    {
        shared_memory sp_shm("/cppev_test_shm_lock", sizeof(TestStruct), false);
        TestStruct *c_ptr = reinterpret_cast<TestStruct *>(sp_shm.ptr());

        EXPECT_TRUE(c_ptr->lock.try_wrlock());
        c_ptr->var1 = 1;

        // waiting for father judge
        while (c_ptr->var1 != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        c_ptr->lock.unlock();

        _exit(0);
    }
    else
    {
        // waiting for child wrlock
        while (f_ptr->var1 != 1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        EXPECT_FALSE(f_ptr->lock.try_rdlock());
        EXPECT_FALSE(f_ptr->lock.try_wrlock());

        f_ptr->var1 = 0;

        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);

        shm.destructor<TestStruct>();
        shm.unlink();
    }
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}

