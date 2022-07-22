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
    bool ready1 = false;
    bool ready2 = false;

    std::thread creater([&]() {
        std::unique_ptr<semaphore> psem;
        {
            std::unique_lock<std::mutex> lg(lock);
            psem = std::unique_ptr<semaphore>(new semaphore(sem_name, sem_value));
            ready1 = true;
        }
        cv.notify_one();
        {
            std::unique_lock<std::mutex> lg(lock);
            if (!ready2)
            {
                cv.wait(lg, [&ready2]() { return ready2; });
            }
        }
        psem->release();
    });

    std::thread user([&]() {
        {
            std::unique_lock<std::mutex> lg(lock);
            if (!ready1)
            {
                cv.wait(lg, [&ready1]() { return ready1; });
            }
        }
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

        {
            std::unique_lock<std::mutex> lg(lock);
            ready2 = true;
        }
        cv.notify_one();

        EXPECT_TRUE(sem.acquire());
        sem.unlink();
    });

    creater.join();
    user.join();
}


}

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}