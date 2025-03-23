#include <gtest/gtest.h>

#include <thread>

#include "config.h"
#include "cppev/ipc.h"
#include "cppev/lock.h"
#include "cppev/logger.h"

namespace cppev
{

class TestIpcByFork : public testing::Test
{
protected:
    TestIpcByFork() : name_("/cppev_test_ipc_name"), name_2_(name_ + "_2")
    {
    }

    std::string name_;

    std::string name_2_;
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

const int delay = 100;

TEST_F(TestIpcByFork, test_sem_shm_move)
{
    std::vector<semaphore> sem_vec;
    sem_vec.emplace_back(name_);
    EXPECT_TRUE(sem_vec[0].creator());
    sem_vec[0].unlink();
    sem_vec.pop_back();

    std::vector<shared_memory> shm_vec;
    shm_vec.emplace_back(name_, 66);
    EXPECT_TRUE(shm_vec[0].creator());
    shm_vec[0].unlink();
    shm_vec.clear();
}

TEST_F(TestIpcByFork, test_sem_shm_by_fork)
{
    int shm_size = 12;

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
        TestStruct(int var1, double var2) : var1(var1), var2(var2)
        {
        }

        rwlock lock{sync_level::process};
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
        TestStruct() : var(0), ready(false)
        {
        }

        mutex lock{sync_level::process};
        cond cv{sync_level::process};
        int var;
        bool ready;
    };

    const int NUMBER = 100;
    pid_t pid = fork();
    if (pid < 0)
    {
        throw_system_error("fork error");
    }
    else if (pid == 0)
    {
        semaphore sem(name_);
        semaphore sem_2(name_2_);
        shared_memory shm(name_, sizeof(TestStruct));
        TestStruct *ptr = shm.construct<TestStruct>();
        ASSERT_EQ(ptr, reinterpret_cast<TestStruct *>(shm.ptr()));
        ASSERT_TRUE(sem.creator());
        ASSERT_TRUE(sem_2.creator());
        ASSERT_TRUE(shm.creator());

        // Test-1
        sem.release();
        printf("test_sem_shm_lock_cond_by_fork process-1 test-1\n");

        {
            std::unique_lock<mutex> lock(ptr->lock);
            ptr->var = NUMBER;
            ptr->ready = true;
            ptr->cv.notify_one();
            ptr->cv.wait(lock);
        }

        ASSERT_TRUE(ptr->lock.try_lock());
        ptr->lock.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(delay));

        for (int i = 0; i < 50; ++i)
        {
            std::unique_lock<mutex> lock(ptr->lock);
            std::cv_status status =
                ptr->cv.wait_for(lock, std::chrono::milliseconds(10));
            ASSERT_EQ(status, std::cv_status::timeout);
        }

        // Test-2
        ptr->ready = false;
        sem.release();
        printf("test_sem_shm_lock_cond_by_fork process-1 test-2\n");

        {
            std::unique_lock<mutex> lock(ptr->lock);
            ptr->ready = true;
            ptr->cv.notify_one();
            std::cv_status status =
                ptr->cv.wait_for(lock, std::chrono::milliseconds(delay * 3));
            ASSERT_EQ(status, std::cv_status::no_timeout);
        }

        // Test-3
        auto pred = [ptr, &NUMBER]() { return ptr->var == NUMBER; };

        ptr->var = NUMBER + 1;
        ptr->ready = false;
        sem.release();
        printf("test_sem_shm_lock_cond_by_fork process-1 test-3\n");

        {
            std::unique_lock<mutex> lock(ptr->lock);
            ptr->ready = true;
            ptr->cv.notify_all();
            bool success = ptr->cv.wait_for(
                lock, std::chrono::milliseconds(delay * 3), pred);
            ASSERT_TRUE(success);
        }

        // // Test-4
        ptr->var = NUMBER + 1;
        ptr->ready = false;
        ASSERT_FALSE(pred());
        sem.release();
        printf("test_sem_shm_lock_cond_by_fork process-1 test-4\n");

        {
            std::unique_lock<mutex> lock(ptr->lock);
            ptr->ready = true;
            ptr->cv.notify_all();
            auto start = std::chrono::system_clock::now();
            bool success =
                ptr->cv.wait_for(lock, std::chrono::milliseconds(delay), pred);
            auto end = std::chrono::system_clock::now();
            std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(
                             (end - start))
                             .count()
                      << std::endl;
            ASSERT_FALSE(success);
        }

        // Finish
        sem_2.acquire();
        printf("test_sem_shm_lock_cond_by_fork process-1 finish\n");
        ptr->~TestStruct();
        shm.unlink();
        sem.unlink();
        sem_2.unlink();
        _exit(0);
    }
    else
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2 * delay));
        semaphore sem(name_);

        // Test-1
        sem.acquire();
        semaphore sem_2(name_2_);
        shared_memory shm(name_, sizeof(TestStruct));
        TestStruct *ptr = reinterpret_cast<TestStruct *>(shm.ptr());
        printf("test_sem_shm_lock_cond_by_fork process-2 test-1\n");

        {
            std::unique_lock<mutex> lock(ptr->lock);
            if (!ptr->ready)
            {
                ptr->cv.wait(lock);
            }
            ASSERT_EQ(ptr->var, NUMBER);
            ptr->cv.notify_one();
        }

        // Test-2
        sem.acquire();
        printf("test_sem_shm_lock_cond_by_fork process-2 test-2\n");

        {
            std::unique_lock<mutex> lock(ptr->lock);
            if (!ptr->ready)
            {
                ptr->cv.wait(lock);
            }
            ASSERT_TRUE(ptr->ready);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            ptr->cv.notify_one();
        }

        // Test-3
        sem.acquire();
        printf("test_sem_shm_lock_cond_by_fork process-2 test-3\n");

        {
            std::unique_lock<mutex> lock(ptr->lock);
            if (!ptr->ready)
            {
                ptr->cv.wait(lock);
            }
            ASSERT_TRUE(ptr->ready);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            ptr->var = NUMBER;
            ptr->cv.notify_all();
        }

        // // Test-4
        sem.acquire();
        printf("test_sem_shm_lock_cond_by_fork process-2 test-4\n");

        {
            std::unique_lock<mutex> lock(ptr->lock);
            if (!ptr->ready)
            {
                ptr->cv.wait(lock);
            }
            ASSERT_TRUE(ptr->ready);
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(delay * 3));
            lock.lock();
            ptr->var = NUMBER;
            ptr->cv.notify_all();
        }

        // Finish
        sem_2.release();
        int ret = -1;
        waitpid(pid, &ret, 0);
        printf("test_sem_shm_lock_cond_by_fork process-2 finish\n");
        ASSERT_EQ(ret, 0);
    }
}

TEST_F(TestIpcByFork, test_shm_one_time_fence_barrier_by_fork)
{
    struct TestStruct : public TestStructBase
    {
        TestStruct() : br(sync_level::process, 2), var(0)
        {
        }

        one_time_fence otf{sync_level::process};
        barrier br;
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

        ptr->otf.wait();
        EXPECT_EQ(ptr->var, 100);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ptr->br.wait();
        LOG_INFO << "Process " << getpid() << " passed barrier";

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
        ptr->otf.notify();

        ptr->otf.wait();

        ptr->br.wait();
        LOG_INFO << "Process " << getpid() << " passed barrier";

        EXPECT_THROW(ptr->br.wait(), std::logic_error);

        if (shm.creator())
        {
            shm.unlink();
        }
        int ret = -1;
        waitpid(pid, &ret, 0);
        EXPECT_EQ(ret, 0);
    }
}

TEST(TestPSharedLockByThread, test_mutex_shm_performance)
{
    std::string shm_name = "/cppev_test_lock_shm";
    shared_memory shm(shm_name, sizeof(mutex));
    mutex *lock_ptr = shm.construct<mutex>(sync_level::process);
    performance_test<mutex>(*lock_ptr);
    lock_ptr->~mutex();
    shm.unlink();
}

}  // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
