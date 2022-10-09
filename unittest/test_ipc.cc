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

TEST_F(TestIpc, test_shm_by_thread)
{
    int shm_size = 128;
    std::string str{"hello\n"};

    std::mutex lock;
    std::condition_variable cv;
    bool ready = false;

    std::thread thr_writer([&]() {
        std::unique_lock<std::mutex> lg(lock);
        shared_memory shm(name, shm_size, true);
        memcpy(shm.ptr(), str.c_str(), str.size());
        ready = true;
        cv.notify_one();
    });

    std::thread thr_reader([&]() {
        {
            std::unique_lock<std::mutex> lg(lock);
            if (!ready) {
                cv.wait(lg, [&ready]() { return ready; });
            }
        }
        shared_memory shm(name, str.size(), false);
        EXPECT_EQ(std::string(reinterpret_cast<char *>(shm.ptr())), str);
        shm.unlink();
    });

    thr_writer.join();
    thr_reader.join();
}

TEST_F(TestIpc, test_sem_by_thread)
{
    int sem_value = 3;

    std::mutex lock;
    std::condition_variable cv;
    bool ready = false;
    int delay = 100;
    const int MAGIC = 666;
    int magic_num = MAGIC - 1;

    using TIME_ACCURACY = std::chrono::microseconds;

    std::thread creater([&]() {
        std::unique_ptr<semaphore> psem;
        {
            std::unique_lock<std::mutex> lg(lock);
            psem = std::unique_ptr<semaphore>(new semaphore(name, sem_value));
            ready = true;
            cv.notify_one();
        }
        std::this_thread::sleep_for(TIME_ACCURACY(delay));
        magic_num = MAGIC;
        psem->release();
    });

    std::thread user([&]() {
        {
            std::unique_lock<std::mutex> lg(lock);
            if (!ready)
            {
                cv.wait(lg, [&ready]() { return ready; });
            }
        }
        int64_t begin = std::chrono::duration_cast<TIME_ACCURACY>
            (std::chrono::system_clock::now().time_since_epoch()).count();
        semaphore sem(name);

        for (int i = 0; i < sem_value; ++i)
        {
            EXPECT_TRUE(sem.acquire());
        }

        EXPECT_FALSE(sem.try_acquire());
        sem.release();
        EXPECT_TRUE(sem.acquire());
        sem.release();
        EXPECT_TRUE(sem.try_acquire());

        EXPECT_TRUE(sem.acquire());
        int64_t end = std::chrono::duration_cast<TIME_ACCURACY>
            (std::chrono::system_clock::now().time_since_epoch()).count();
        EXPECT_EQ(magic_num, MAGIC);
        EXPECT_GE(end - begin, delay);
        sem.unlink();
    });

    creater.join();
    user.join();
}

TEST_F(TestIpc, test_shm_sem_by_fork)
{
    shared_memory shm(name, 128, true);
    semaphore sem(name, 1);
    memcpy(shm.ptr(), "cppev", 5);
    sem.release();

    pid_t pid = fork();
    if (pid < 0)
    {
        throw_system_error("fork error");
    }

    if (pid == 0)
    {
        shared_memory sp_shm(name, 128, false);
        semaphore sp_sem(name);

        EXPECT_EQ(std::string(reinterpret_cast<char *>(shm.ptr())), "cppev");
        std::cout << "shared memory ptr : " << shm.ptr() << std::endl;
        EXPECT_TRUE(sp_sem.acquire());
        EXPECT_TRUE(sp_sem.try_acquire());
        EXPECT_TRUE(sp_sem.acquire());
        EXPECT_FALSE(sp_sem.try_acquire());
        sp_shm.unlink();
        sp_sem.unlink();
        std::cout << "end of child process" << std::endl;
        _exit(0);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        sem.release();
        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);
    }
}

TEST_F(TestIpc, test_shm_rwlock_by_fork)
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

    shared_memory shm(name, sizeof(TestStruct), true);
    TestStruct *f_ptr = shm.construct<TestStruct, int, double>(0, 6.6);
    EXPECT_EQ(reinterpret_cast<void *>(f_ptr), shm.ptr());

    EXPECT_TRUE(f_ptr->lock.try_wrlock());
    f_ptr->lock.unlock();
    EXPECT_TRUE(f_ptr->lock.try_rdlock());
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

        EXPECT_TRUE(c_ptr->lock.try_rdlock());
        c_ptr->lock.unlock();

        _exit(0);
    }

    EXPECT_TRUE(f_ptr->lock.try_rdlock());
    f_ptr->lock.unlock();

    int ret = -1;
    waitpid(pid, &ret, 0);
    EXPECT_EQ(ret, 0);

    f_ptr->~TestStruct();
    shm.unlink();
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

    shared_memory shm(name, sizeof(TestStruct), true);
    TestStruct *f_ptr = shm.construct<TestStruct>();
    EXPECT_EQ(f_ptr, reinterpret_cast<TestStruct *>(shm.ptr()));
    int NUMBER = 100;

    pid_t pid = fork();
    if (pid < 0)
    {
        throw_system_error("fork error");
    }

    if (pid == 0)
    {
        shared_memory sub_shm(name, sizeof(TestStruct), false);
        TestStruct *c_ptr = reinterpret_cast<TestStruct *>(sub_shm.ptr());
        EXPECT_NE(shm.ptr(), sub_shm.ptr());
        EXPECT_NE(f_ptr, c_ptr);

        {
            std::unique_lock<pshared_lock> lock(c_ptr->lock);
            c_ptr->var = NUMBER;
            c_ptr->ready = true;
            c_ptr->cond.notify_one();
            c_ptr->cond.wait(lock);
        }

        EXPECT_TRUE(c_ptr->lock.try_lock());
        c_ptr->lock.unlock();

        c_ptr->~TestStruct();
        sub_shm.unlink();

        _exit(0);
    }

    {
        std::unique_lock<pshared_lock> lock(f_ptr->lock);
        if(!f_ptr->ready)
        {
            f_ptr->cond.wait(lock);
        }

        EXPECT_EQ(f_ptr->var, NUMBER);

        f_ptr->cond.notify_one();
    }

    int ret = -1;
    waitpid(pid, &ret, 0);
    EXPECT_EQ(ret, 0);
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
