#include <gtest/gtest.h>
#include "cppev/runnable.h"
#include "cppev/thread_pool.h"
#include <chrono>

using namespace std;
using namespace cppev;
using namespace testing;

using chrono::duration;
using chrono::duration_cast;

class runnable_tester: public runnable {
public:
    static const int sleep_time;

    void run_impl() override {
        this_thread::sleep_for(chrono::milliseconds(sleep_time));
    }
};

const int runnable_tester::sleep_time = 1;

int thr_count = 0;
mutex m;

class test_tp : public Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST(test_tp, test_tp0) {
    thread_pool<runnable_tester> tp(50);
    tp.run();
    auto start = chrono::high_resolution_clock::now();
    tp.join();
    auto end = chrono::high_resolution_clock::now();
    duration<double> d = duration_cast<duration<double> >(end - start);
    ASSERT_GT(d.count(), (runnable_tester::sleep_time * 0.9) / 1000.0);
}

TEST(test_tp, test_tp1) {
    int count = 100;

    auto f = [](void *) {
        unique_lock<mutex> lock(m);
        thr_count++;
    };

    shared_ptr<tp_task> t(new tp_task(f, nullptr));

    thread_pool_queue tp(50);
    tp.run();
    for (int i = 0; i < count; ++i) {
        tp.add_task(t);
    }
    tp.stop();
    ASSERT_EQ(thr_count, count);
}

int main(int argc, char **argv) {
    InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}