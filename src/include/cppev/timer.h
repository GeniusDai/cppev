#ifndef _timer_h_6C0224787A17_
#define _timer_h_6C0224787A17_

#include <cstddef>
#include <functional>
#include <tuple>
#include <vector>
#include <queue>
#include <chrono>
#include <memory>
#include <string>
#include <map>
#include <thread>
#include <algorithm>
#include <ctime>
#include <csignal>
#include "cppev/utils.h"

namespace cppev
{
// @param ts_curr : trigger timestamp
using timer_handler = std::function<void(const std::chrono::nanoseconds &ts_curr)>;

// @param ts_curr : trigger timestamp
// @param ts_next : next trigger timestamp
using discrete_handler = std::function<void(const std::chrono::nanoseconds &ts_curr,
    const std::chrono::nanoseconds &ts_next)>;

template<typename Clock = std::chrono::system_clock>
class timed_task_executor final
{
public:
    explicit timed_task_executor(const std::chrono::nanoseconds &span, const timer_handler &handler)
    : handler_(handler), span_(span), curr_(Clock::now()), stop_ (false)
    {
        auto thr_func = [this]()
        {
            while(!stop_)
            {
                curr_ += std::chrono::duration_cast<typename decltype(curr_)::duration>(span_);
                std::this_thread::sleep_until(curr_);
                handler_(curr_.time_since_epoch());
            }
        };
        thr_ = std::thread(thr_func);
    }

    timed_task_executor(const timed_task_executor &) = delete;
    timed_task_executor &operator=(const timed_task_executor &) = delete;
    timed_task_executor(timed_task_executor &&) = delete;
    timed_task_executor &operator=(timed_task_executor &&) = delete;

    ~timed_task_executor() noexcept
    {
        stop_ = true;
        thr_.join();
    }

private:
    timer_handler handler_;

    std::chrono::nanoseconds span_;

    std::chrono::time_point<Clock> curr_;

    bool stop_;

    std::thread thr_;
};



template<typename Clock = std::chrono::system_clock>
class timed_multitask_executor
{
private:
    template<typename T, std::size_t I>
    struct tasks_less_cmp
    {
        bool operator()(const T &lhs, const T &rhs) const noexcept
        {
            return static_cast<int>(std::get<I>(lhs)) < static_cast<int>(std::get<I>(rhs));
        }
    };
public:
    // Create backend thread to execute tasks
    // @param timer_tasks : tasks that will be triggered regularly according to frequency.
    //                      tuple : <frequency, priority, callback>
    // @param discrete_tasks : tasks that will be triggered between timed_task_executor tasks.
    //                         tuple : <priority, callback>
    // @param safety_factor : discrete tasks will not be executed if time before next trigger
    //                        is shorter than the interval * safety_factor
    // @param align : whether start time align to 1s.
    timed_multitask_executor(
        const std::vector<std::tuple<double, priority, timer_handler>> &timer_tasks,
        const std::vector<std::tuple<priority, discrete_handler>> &discrete_tasks,
        double safety_factor = 0.1,
        bool align = true
    )
    : stop_(false)
    {
        std::priority_queue<std::tuple<priority, size_t>, std::vector<std::tuple<priority, size_t>>,
            tasks_less_cmp<std::tuple<priority, size_t>, 0>> timer_tasks_heap, discrete_tasks_heap;
        for (size_t i = 0; i < timer_tasks.size(); ++i)
        {
            timer_tasks_heap.push({ std::get<1>(timer_tasks[i]) , i });
        }

        for (size_t i = 0; i < discrete_tasks.size(); ++i)
        {
            discrete_tasks_heap.push({ std::get<0>(discrete_tasks[i]) , i });
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

        while (!discrete_tasks_heap.empty())
        {
            size_t idx = std::get<1>(discrete_tasks_heap.top());
            discrete_tasks_heap.pop();
            discrete_handlers_.push_back(std::get<1>(discrete_tasks[idx]));
        }

        thr_ = std::thread(
            [this, align, safety_factor]()
            {
                auto tp_curr = Clock::now();
                if (align)
                {
                    tp_curr = ceil_time_point(tp_curr);
                }
                while(!this->stop_)
                {
                    for (auto iter = this->tasks_.cbegin(); iter != this->tasks_.cend(); )
                    {
                        auto curr = iter;
                        auto next = ++iter;

                        int64_t span;
                        if (next == this->tasks_.cend())
                        {
                            span = this->interval_ - curr->first;
                        }
                        else
                        {
                            span = next->first - curr->first;
                        }

                        for (auto idx : curr->second)
                        {
                            this->timer_handlers_[idx](tp_curr.time_since_epoch());
                        }

                        auto safety_time_point = tp_curr.time_since_epoch() +
                            std::chrono::nanoseconds((int64_t)(span * (1 - safety_factor)));

                        auto tp_next = tp_curr + std::chrono::nanoseconds(span);

                        for (const auto &h : this->discrete_handlers_)
                        {
                            if (Clock::now().time_since_epoch() > safety_time_point)
                            {
                                break;
                            }
                            h(tp_curr.time_since_epoch(), tp_next.time_since_epoch());
                        }

                        std::this_thread::sleep_until(tp_next);

                        tp_curr = std::chrono::time_point_cast<
                            typename decltype(tp_curr)::duration>(tp_next);
                    }
                }
            }
        );
    }

    timed_multitask_executor(const timed_multitask_executor &) = delete;
    timed_multitask_executor &operator=(const timed_multitask_executor &) = delete;
    timed_multitask_executor(timed_multitask_executor &&) = delete;
    timed_multitask_executor &operator=(timed_multitask_executor &&) = delete;

    ~timed_multitask_executor()
    {
        stop_ = true;
        thr_.join();
    }

private:
    // whether thread shall stop
    bool stop_;

    // time interval in nanoseconds
    int64_t interval_;

    // timed_task_executor task handlers
    std::vector<timer_handler> timer_handlers_;

    // discrete task handlers
    std::vector<discrete_handler> discrete_handlers_;

    // tasks :
    //    timepoint -> timed_task_executor handler index
    std::map<int64_t, std::vector<size_t>> tasks_;

    // backend thread execute tasks
    std::thread thr_;
};


}   // namespace cppev

#endif  // timed_task_executor.h
