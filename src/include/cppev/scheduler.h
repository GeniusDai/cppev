#ifndef _scheduler_h_6C0224787A17_
#define _scheduler_h_6C0224787A17_

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
#include "cppev/utils.h"

namespace cppev
{

enum task_status
{
    done,
    yield,
};

// Triggered once when thread starts
using init_task_handler = std::function<void(void)>;

// Triggered once when thread exits
using exit_task_handler = std::function<void(void)>;

// Triggered periodically by timed_scheduler
// @param ts_curr : trigger timestamp
using timed_task_handler = std::function<void(const std::chrono::nanoseconds &ts_curr)>;

// Triggered periodically by timed_scheduler
// @param ts_curr : trigger timestamp
// @param ts_next : next trigger timestamp
// @return : task status
using discrete_task_handler = std::function<task_status(const std::chrono::nanoseconds &ts_curr,
    const std::chrono::nanoseconds &ts_next)>;

// Triggered periodically by timesharing_scheduler
// @param timeslice : time slice the task has been assigned
// @return : task status
using timesharing_task_handler = std::function<task_status(const std::chrono::nanoseconds &timeslice)>;

// Triggered periodically by timesharing_scheduler
using yield_scheduler_handler = std::function<void(void)>;



template<typename Clock = std::chrono::system_clock>
class timed_scheduler
{
    static constexpr double default_safety_factor = 0.1;
    static constexpr std::chrono::nanoseconds default_safety_span = std::chrono::microseconds(100);
public:
    // Create backend thread to execute tasks
    // @param timer_tasks : tasks that will be triggered regularly according to frequency.
    //                      tuple : <frequency, priority, callback>
    // @param discrete_tasks : tasks that will be triggered between timed_task_executor tasks.
    //                         tuple : <priority, callback>
    // @param init_tasks : tasks that will be executed once only when thread starts
    // @param exit_tasks : tasks that will be executed once only when thread exits
    // @param safety_factor : discrete tasks will not be executed if time before next trigger
    //                        is shorter than interval * safety_factor
    // @param safety_span : discrete task will not be executed if timer before next trigger
    //                      is shorter than safety_span
    // @param align : whether start time align to 1s.
    timed_scheduler(
        const std::vector<std::tuple<double, priority, timed_task_handler>> &timer_tasks,
        const std::vector<std::tuple<priority, discrete_task_handler>> &discrete_tasks = {},
        const std::vector<init_task_handler> &init_tasks = {},
        const std::vector<exit_task_handler> &exit_tasks = {},
        const double safety_factor = default_safety_factor,
        const std::chrono::nanoseconds &safety_span = default_safety_span,
        const bool align = true
    )
    : stop_(false)
    {
        std::priority_queue<std::tuple<priority, size_t>, std::vector<std::tuple<priority, size_t>>,
            tuple_less<std::tuple<priority, size_t>, 0>> timer_tasks_heap, discrete_tasks_heap;
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
            discrete_task_handlers_.push_back(std::get<1>(discrete_tasks[idx]));
        }

        thr_ = std::thread(
            [this, init_tasks, exit_tasks, safety_factor, safety_span, align]()
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
                while(!stop_)
                {
                    for (auto iter = tasks_.cbegin(); !stop_ && (iter != tasks_.cend()); )
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

                        auto safety_interval = std::max(safety_span, std::chrono::nanoseconds(
                            static_cast<int64_t>(span * safety_factor)));

                        auto safety_time_point = tp_next.time_since_epoch() - safety_interval;

                        bool shall_stop_loop = false;

                        for (const auto &task : discrete_task_handlers_)
                        {
                            while (true)
                            {
                                if (stop_ || (Clock::now().time_since_epoch() > safety_time_point))
                                {
                                    shall_stop_loop = true;
                                    break;
                                }
                                if (task(tp_curr.time_since_epoch(), tp_next.time_since_epoch()) ==
                                    task_status::done)
                                {
                                    break;
                                }
                            }
                            if (shall_stop_loop)
                            {
                                break;
                            }
                        }

                        std::this_thread::sleep_until(tp_next);

                        tp_curr = std::chrono::time_point_cast<
                            typename decltype(tp_curr)::duration>(tp_next);
                    }
                }
                for (const auto &task : exit_tasks)
                {
                    task();
                }
            }
        );
    }

    timed_scheduler(
        const double freq,
        const timed_task_handler &handler,
        const discrete_task_handler &discrete_task,
        const init_task_handler &init_task,
        const exit_task_handler &exit_task,
        const double safety_factor = default_safety_factor,
        const std::chrono::nanoseconds &safety_span = default_safety_span,
        const bool align = true
    )
    : timed_scheduler({{ freq, priority::p0, handler }}, {{ priority::p0, discrete_task }},
        { init_task }, { exit_task }, safety_factor, safety_span, align)
    {
    }

    timed_scheduler(
        const double freq,
        const timed_task_handler &handler,
        const bool align = true
    )
    : timed_scheduler({{ freq, priority::p0, handler }}, {}, {}, {},
        default_safety_factor, default_safety_span, align)
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

    // discrete task handlers, discending priority order
    std::vector<discrete_task_handler> discrete_task_handlers_;

    // timepoint -> timed handler index
    std::map<int64_t, std::vector<size_t>> tasks_;

    // backend thread executing tasks
    std::thread thr_;
};



class timesharing_scheduler
{
    using clock = std::chrono::steady_clock;

    using task_queue_type = std::priority_queue<
        std::tuple<priority, std::chrono::nanoseconds, std::shared_ptr<timesharing_task_handler>>,
        std::vector<std::tuple<priority, std::chrono::nanoseconds, std::shared_ptr<timesharing_task_handler>>>,
        tuple_less<std::tuple<priority, std::chrono::nanoseconds, std::shared_ptr<timesharing_task_handler>>, 0>
    >;
public:
    // Create backend thread to execute tasks
    // @param timesharing_tasks : tasks that will be executed with awareness of timeslice
    // @param yield_scheduler : task that will be triggered when a round of scheduling finishes
    // @param timeslice_unit : unit of time slice for tasks
    // @param init_tasks : tasks that will be executed once only when thread starts
    // @param exit_tasks : tasks that will be executed once only when thread exits
    timesharing_scheduler(
        const std::vector<std::tuple<priority, size_t, timesharing_task_handler>> &timesharing_tasks,
        const yield_scheduler_handler &yield_scheduler,
        const std::chrono::nanoseconds &timeslice_unit,
        const std::vector<init_task_handler> &init_tasks = {},
        const std::vector<exit_task_handler> &exit_tasks = {}
    )
    : stop_(false)
    {
        for (const auto &task : timesharing_tasks)
        {
            all_tasks_.push(std::make_tuple(std::get<0>(task), timeslice_unit * std::get<1>(task),
                std::make_shared<timesharing_task_handler>(std::get<2>(task))));
        }

        thr_ = std::thread(
            [=]()
            {
                for (const auto &task : init_tasks)
                {
                    task();
                }

                task_queue_type curr_tasks = all_tasks_;
                task_queue_type next_tasks;

                while (!stop_)
                {
                    while(!curr_tasks.empty() && !stop_)
                    {
                        auto task = curr_tasks.top();
                        curr_tasks.pop();

                        auto tp_begin = clock::now();
                        auto status = (*std::get<2>(task))(std::get<1>(task));
                        if (status == task_status::done)
                        {
                            continue;
                        }
                        auto tp_end = clock::now();

                        auto remain = std::get<1>(task) - std::chrono::duration_cast<std::chrono::nanoseconds>
                            (tp_end.time_since_epoch() - tp_begin.time_since_epoch());

                        if (remain.count() > 0)
                        {
                            next_tasks.push(std::make_tuple(std::get<0>(task), remain, std::get<2>(task)));
                        }
                    }
                    if (stop_)
                    {
                        break;
                    }
                    else if (next_tasks.empty())
                    {
                        curr_tasks = all_tasks_;
                        yield_scheduler();
                    }
                    else
                    {
                        curr_tasks.swap(next_tasks);
                    }
                }

                for (const auto &task : exit_tasks)
                {
                    task();
                }
            }
        );
    }

    timesharing_scheduler(const timesharing_scheduler &) = delete;
    timesharing_scheduler &operator=(const timesharing_scheduler &) = delete;
    timesharing_scheduler(timesharing_scheduler &&) = delete;
    timesharing_scheduler &operator=(timesharing_scheduler &&) = delete;

    ~timesharing_scheduler()
    {
        stop_ = true;
        thr_.join();
    }

private:
    // whether thread shall stop
    bool stop_;

    // All the tasks
    task_queue_type all_tasks_;

    // backend thread executing tasks
    std::thread thr_;
};

}   // namespace cppev

#endif  // scheduler.h
