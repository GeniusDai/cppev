#ifndef _cppev_unittest_config_h_6C0224787A17_
#define _cppev_unittest_config_h_6C0224787A17_

#include <mutex>
#include <thread>
#include <vector>

namespace cppev
{

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

}  // namespace cppev

#endif  // config.h
