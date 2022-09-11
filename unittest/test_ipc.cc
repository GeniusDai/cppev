#include <gtest/gtest.h>
#include "cppev/ipc.h"
#include <thread>
#include <mutex>
#include <condition_variable>

namespace cppev
{

class TestIpc
: public testing::Test
{
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}
};

TEST_F(TestIpc, test_shm)
{
    std::string shm_name{"/cppev_test_shm"};
    int shm_size = 128;
    std::string str{"hello\n"};

    std::mutex lock;
    std::condition_variable cv;
    bool ready = false;

    std::thread thr_writer([&]() {
        {
            std::unique_lock<std::mutex> lg(lock);
            shared_memory shm(shm_name, shm_size, true);
            memcpy(shm.ptr(), str.c_str(), str.size());
            ready = true;
        }
        cv.notify_one();
    });

    std::thread thr_reader([&]() {
        {
            std::unique_lock<std::mutex> lg(lock);
            if (!ready) {
                cv.wait(lg, [&ready]() { return ready; });
            }
        }
        shared_memory shm(shm_name, str.size(), false);
        EXPECT_EQ(std::string(reinterpret_cast<char *>(shm.ptr())), str);
        shm.unlink();
    });

    thr_writer.join();
    thr_reader.join();
}

TEST_F(TestIpc, test_sem)
{
    std::string sem_name{"/cppev_test_sem"};
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
            psem = std::unique_ptr<semaphore>(new semaphore(sem_name, sem_value));
            ready = true;
        }
        cv.notify_one();
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
        semaphore sem(sem_name);

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

TEST_F(TestIpc, test_by_fork)
{
    std::string name{"/cppev_test_name"};
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

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
