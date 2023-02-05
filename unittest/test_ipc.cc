#include <thread>
#include <mutex>
#include <condition_variable>
#include <gtest/gtest.h>
#include "cppev/ipc.h"
#include "cppev/lock.h"

namespace cppev
{

class TestIpc
: public testing::Test
{
protected:
    TestIpc()
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

TEST_F(TestIpc, test_sem_shm_by_fork)
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
        std::this_thread::sleep_for(std::chrono::microseconds(200));

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

TEST_F(TestIpc, test_sem_shm_rwlock_by_fork)
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

        semaphore sem(name_);
        sem.release();

        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);
    }
}

TEST_F(TestIpc, test_sem_shm_lock_cond_by_fork)
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

TEST_F(TestIpc, test_shm_one_time_fence_barrier_by_fork)
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

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
