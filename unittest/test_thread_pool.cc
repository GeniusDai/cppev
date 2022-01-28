#include <gtest/gtest.h>
#include "cppev/runnable.h"
#include "cppev/thread_pool.h"
#include <chrono>

namespace cppev {

class runnable_tester: public runnable {
public:
    static const int sleep_time;

    void run_impl() override {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    }
};

const int runnable_tester::sleep_time = 1;

int thr_count = 0;
std::mutex m;

class TestThreadPool : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST(TestThreadPool, test_tp0) {
    thread_pool<runnable_tester> tp(50);
    tp.run();
    auto start = std::chrono::high_resolution_clock::now();
    tp.join();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> d = std::chrono::duration_cast
        <std::chrono::duration<double> >(end - start);
    ASSERT_GT(d.count(), (runnable_tester::sleep_time * 0.9) / 1000.0);
}

TEST(TestThreadPool, test_tp1) {
    int count = 100;

    auto f = [](void *) {
        std::unique_lock<std::mutex> lock(m);
        thr_count++;
    };

    std::shared_ptr<tp_task> t(new tp_task(f, nullptr));

    thread_pool_queue tp(50);
    tp.run();
    for (int i = 0; i < count; ++i) {
        tp.add_task(t);
    }
    tp.stop();
    ASSERT_EQ(thr_count, count);
}

}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}