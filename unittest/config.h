#include <mutex>
#include <thread>
#include <vector>

template <typename Mutex>
void performance_test(Mutex &lock)
{
    int count = 0;

    int add_num = 50000;
    int thr_num = 50;

    auto task = [&]()
    {
        for (int i = 0; i < add_num; ++i)
        {
            std::unique_lock<Mutex> _(lock);
            ++count;
        }
    };

    std::vector<std::thread> thrs;
    for (int i = 0; i < thr_num; ++i)
    {
        thrs.emplace_back(task);
    }
    for (int i = 0; i < thr_num; ++i)
    {
        thrs[i].join();
    }

    EXPECT_EQ(count, add_num * thr_num);
}
