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
    : name("/cppev_test_ipc_name")
    {}

    void SetUp() override
    {}

    void TearDown() override
    {}

    std::string name;
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

TEST_F(TestIpc, test_shm_sem_by)
{
    int shm_size = 12;
    pid_t pid = fork();
    if (pid < 0)
    {
        throw_system_error("fork error");
    }
    else if (pid == 0)
    {
        semaphore sem(name);
        sem.acquire();

        shared_memory shm(name, shm_size);
        EXPECT_EQ(std::string(reinterpret_cast<char *>(shm.ptr())), "cppev");
        EXPECT_FALSE(sem.try_acquire());
        sem.release();
        EXPECT_TRUE(sem.try_acquire());

        std::cout << "shared memory ptr : " << shm.ptr() << std::endl;

        if (shm.creator())
        {
            shm.unlink();
        }
        if (sem.creator())
        {
            sem.unlink();
        }

        std::cout << "end of child process" << std::endl;
        _exit(0);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        shared_memory shm(name, shm_size);
        semaphore sem(name);
        memcpy(shm.ptr(), "cppev", 5);

        sem.release();

        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);

        if (shm.creator())
        {
            shm.unlink();
        }
        if (sem.creator())
        {
            sem.unlink();
        }
    }
}

TEST_F(TestIpc, test_shm_rwlock)
{
    struct TestStruct : public TestStructBase
    {
        TestStruct(int var1, double var2)
        : var1(var1), var2(var2)
        {}

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
        semaphore sem(name);
        sem.acquire();
        sem.unlink();

        shared_memory shm(name, sizeof(TestStruct));
        TestStruct *ptr = reinterpret_cast<TestStruct *>(shm.ptr());

        EXPECT_TRUE(ptr->lock.try_rdlock());
        ptr->lock.unlock();

        ptr->~TestStruct();
        shm.unlink();

        _exit(0);
    }
    else
    {
        shared_memory shm(name, sizeof(TestStruct));
        TestStruct *ptr = shm.construct<TestStruct, int, double>(0, 6.6);
        EXPECT_EQ(reinterpret_cast<void *>(ptr), shm.ptr());

        EXPECT_TRUE(ptr->lock.try_wrlock());
        ptr->lock.unlock();
        EXPECT_TRUE(ptr->lock.try_rdlock());
        ptr->lock.unlock();

        EXPECT_TRUE(ptr->lock.try_rdlock());
        ptr->lock.unlock();

        semaphore sem(name);
        sem.release();

        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);
    }
}

TEST_F(TestIpc, test_shm_lock_cond_by_fork)
{
    struct TestStruct : public TestStructBase
    {
        TestStruct()
        : var(0), ready(false)
        {}

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
        semaphore sem(name);
        sem.acquire();
        sem.unlink();

        shared_memory shm(name, sizeof(TestStruct));
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

        shared_memory shm(name, sizeof(TestStruct));
        TestStruct *ptr = shm.construct<TestStruct>();
        EXPECT_EQ(ptr, reinterpret_cast<TestStruct *>(shm.ptr()));

        semaphore sem(name);
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

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
