#ifndef _cppev_scheduler_h_6C0224787A17_
#define _cppev_scheduler_h_6C0224787A17_

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include "cppev/utils.h"

namespace cppev
{

// Triggered once when thread starts
using init_task_handler = std::function<void(void)>;

// Triggered once when thread exits
using exit_task_handler = std::function<void(void)>;

// Triggered periodically by timed_scheduler
// @param curr_timestamp : current trigger timestamp
using timed_task_handler =
    std::function<void(const std::chrono::nanoseconds &curr_timestamp)>;

template <typename Clock = std::chrono::system_clock>
class CPPEV_PUBLIC timed_scheduler
{
public:
    // Create backend thread to execute tasks
    // @param timer_tasks : tasks that will be triggered regularly according to
    // frequency.
    //                      tuple : <frequency, priority, task>
    // @param init_tasks : tasks that will be executed once only when thread
    // starts
    // @param exit_tasks : tasks that will be executed once only when thread
    // exits
    // @param align : whether start time align to 1s.
    timed_scheduler(
        const std::vector<std::tuple<double, priority, timed_task_handler>>
            &timer_tasks,
        const std::vector<init_task_handler> &init_tasks = {},
        const std::vector<exit_task_handler> &exit_tasks = {},
        const bool align = true)
        : stop_(false)
    {
        std::priority_queue<std::tuple<priority, size_t>,
                            std::vector<std::tuple<priority, size_t>>,
                            tuple_less<std::tuple<priority, size_t>, 0>>
            timer_tasks_heap;
        for (size_t i = 0; i < timer_tasks.size(); ++i)
        {
            timer_tasks_heap.push({std::get<1>(timer_tasks[i]), i});
        }

        std::vector<int64_t> intervals;
        for (size_t i = 0; i < timer_tasks.size(); ++i)
        {
            intervals.push_back(1'000'000'000 / std::get<0>(timer_tasks[i]));
        }
        int64_t lcm = intervals[0];
        int64_t gcd = intervals[0];
        if (intervals.size() >= 2)
        {
            lcm = least_common_multiple(intervals);
            gcd = greatest_common_divisor(intervals);
        }
        interval_ = lcm;

        while (!timer_tasks_heap.empty())
        {
            size_t idx = std::get<1>(timer_tasks_heap.top());
            timer_tasks_heap.pop();
            timer_handlers_.push_back(std::get<2>(timer_tasks[idx]));
            for (int64_t stamp = 0; stamp < lcm; stamp += gcd)
            {
                if (stamp % intervals[idx] == 0)
                {
                    tasks_[stamp].push_back(timer_handlers_.size() - 1);
                }
            }
        }

        thr_ = std::thread(
            [this, init_tasks, exit_tasks, align]()
            {
                for (const auto &task : init_tasks)
                {
                    task();
                }
                auto tp_curr = Clock::now();
                if (align)
                {
                    tp_curr = ceil_time_point<Clock>(tp_curr);
                    std::this_thread::sleep_until(tp_curr);
                }
                while (!stop_)
                {
                    for (auto iter = tasks_.cbegin();
                         !stop_ && (iter != tasks_.cend());)
                    {
                        auto curr = iter;
                        auto next = ++iter;

                        int64_t span;
                        if (next == tasks_.cend())
                        {
                            span = interval_ - curr->first;
                        }
                        else
                        {
                            span = next->first - curr->first;
                        }

                        for (auto idx : curr->second)
                        {
                            timer_handlers_[idx](tp_curr.time_since_epoch());
                        }

                        auto tp_next = tp_curr + std::chrono::nanoseconds(span);

                        std::this_thread::sleep_until(tp_next);

                        tp_curr = std::chrono::time_point_cast<
                            typename decltype(tp_curr)::duration>(tp_next);
                    }
                }
                for (const auto &task : exit_tasks)
                {
                    task();
                }
            });
    }

    timed_scheduler(const double freq, const timed_task_handler &handler,
                    const bool align = true)
        : timed_scheduler({{freq, priority::p0, handler}}, {}, {}, align)
    {
    }

    timed_scheduler(const timed_scheduler &) = delete;
    timed_scheduler &operator=(const timed_scheduler &) = delete;
    timed_scheduler(timed_scheduler &&) = delete;
    timed_scheduler &operator=(timed_scheduler &&) = delete;

    ~timed_scheduler()
    {
        stop_ = true;
        thr_.join();
    }

private:
    // whether thread shall stop
    bool stop_;

    // time interval in nanoseconds
    int64_t interval_;

    // timed task handlers, discending priority order
    std::vector<timed_task_handler> timer_handlers_;

    // timepoint -> timed handler index
    std::map<int64_t, std::vector<size_t>> tasks_;

    // backend thread executing tasks
    std::thread thr_;
};

}  // namespace cppev

#endif  // scheduler.h
