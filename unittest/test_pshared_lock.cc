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

TEST_F(TestLock, test_shm_rwlock)
{
    struct TestStruct
    {
        TestStruct(int var1, double var2, std::string str)
        : var1(var1), var2(var2), str(str)
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
        std::string str;
    };

    std::string name = "/cppev_test_shm_lock";

    shared_memory shm(name, sizeof(TestStruct), true);
    shm.construct<TestStruct, int, double, std::string>(0, 6.6, std::string("hi"));
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
        shared_memory sub_shm(name, sizeof(TestStruct), false);
        TestStruct *c_ptr = reinterpret_cast<TestStruct *>(sub_shm.ptr());

        EXPECT_TRUE(c_ptr->lock.try_wrlock());
        c_ptr->var1 = 1;

        // waiting for father assert
        while (c_ptr->var1 != 0);

        c_ptr->lock.unlock();

        _exit(0);
    }
    else
    {
        // waiting for child wrlock
        while (f_ptr->var1 != 1);

        EXPECT_FALSE(f_ptr->lock.try_rdlock());
        EXPECT_FALSE(f_ptr->lock.try_wrlock());

        f_ptr->var1 = 0;

        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);

        shm.destruct<TestStruct>();
        shm.unlink();
    }
}

TEST_F(TestLock, test_shm_lock_cond)
{
    struct TestStruct
    {
        TestStruct() : var(0), ready(false) {}
        pshared_lock lock;
        pshared_cond cond;
        int var;
        bool ready;
    };

    std::string name = "/cppev_test_shm_lock";

    shared_memory shm(name, sizeof(TestStruct), true);
    shm.construct<TestStruct>();
    TestStruct *f_ptr = reinterpret_cast<TestStruct *>(shm.ptr());
    int num = 100;

    pid_t pid = fork();
    if (pid < 0)
    {
        throw_system_error("fork error");
    }
    else if (pid == 0)
    {
        shared_memory sub_shm(name, sizeof(TestStruct), false);
        TestStruct *c_ptr = reinterpret_cast<TestStruct *>(sub_shm.ptr());
        EXPECT_NE(shm.ptr(), sub_shm.ptr());
        EXPECT_NE(f_ptr, c_ptr);
        for (int i = 0; i < num; ++i)
        {
            while (!c_ptr->ready);  // fence
            std::unique_lock<pshared_lock> lock(c_ptr->lock);
            c_ptr->var = i;
            c_ptr->ready = false;
            c_ptr->cond.notify_one();
        }
        _exit(0);
    }
    else
    {
        int sum = 0;
        for (int i = 0; i < num; ++i)
        {
            std::unique_lock<pshared_lock> lock(f_ptr->lock);
            f_ptr->ready = true;
            f_ptr->cond.wait(lock);
            sum += f_ptr->var;
        }
        EXPECT_EQ(sum, (0 + num - 1) * num / 2);

        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);

        shm.destruct<TestStruct>();

        shm.unlink();
    }
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
