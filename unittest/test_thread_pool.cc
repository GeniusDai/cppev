#include <gtest/gtest.h>
#include "cppev/runnable.h"
#include "cppev/thread_pool.h"
#include <chrono>

namespace cppev
{

const int delay = 100;

class runnable_tester
: public runnable
{
public:
    void run_impl() override
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }
};

class runnable_tester_with_param
: public runnable
{
public:
    runnable_tester_with_param(
        const std::string &,
        std::string,
        std::string &&,
        const char *,
        int64_t,
        void *,
        void const *)
    {
    }

    void run_impl() override
    {
    }
};

TEST(TestThreadPool, test_thread_pool_by_join)
{
    thread_pool<runnable_tester> tp(50);
    tp.run();
    auto start = std::chrono::high_resolution_clock::now();
    tp.join();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> d = std::chrono::duration_cast
        <std::chrono::duration<double> >(end - start);
    ASSERT_GT(d.count(), (delay * 0.9) / 1000.0);
}

TEST(TestThreadPool, test_thread_pool_by_cancel)
{
    thread_pool<runnable_tester> tp(50);
    tp.run();
    tp.cancel();
    tp.join();
}

TEST(TestThreadPool, test_thread_pool_compile_with_param)
{
    std::string str;
    using tp_tester = thread_pool<runnable_tester_with_param,
        const std::string &,
        std::string,
        std::string &&,
        const char *,
        int64_t,
        void *,
        void const *>;
    std::vector<tp_tester> tp_vec;
    tp_vec.emplace_back(10, "", "", std::move(str), "", 0, nullptr, nullptr);
    thread_pool tp1 = std::move(tp_vec[0]);
    thread_pool tp(std::move(tp1));
    tp.run();
    tp.join();
}

TEST(TestThreadPool, test_thread_pool_task_queue)
{
    int count = 1000;
    int sum = 0;
    std::mutex lock;

    auto f = [&]()
    {
        std::unique_lock<std::mutex> lk(lock);
        sum++;
    };

    thread_pool_task_queue tp(50);
    tp.run();
    for (int i = 0; i < count; ++i)
    {
        tp.add_task(f);
    }
    tp.stop();
    ASSERT_EQ(sum, count);
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
