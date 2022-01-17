#ifndef _thread_pool_h_6C0224787A17_
#define _thread_pool_h_6C0224787A17_

#include <vector>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <type_traits>
#include <vector>
#include "cppev/common_utils.h"
#include "cppev/runnable.h"


namespace cppev {

template<typename Runnable, typename... Args>
class thread_pool : public uncopyable {
    static_assert(std::is_base_of<runnable, Runnable>::value);
public:
    thread_pool(int thr_num, Args... args) {
        for (int i = 0; i < thr_num; ++i)
        { thrs_.push_back(std::shared_ptr<Runnable>(new Runnable(args...))); }
    }

    virtual ~thread_pool() {}

    // Run all threads
    void run() { for (auto thr: thrs_) { thr->run(); } }

    // Wait for all threads
    void join() { for (auto thr : thrs_) { thr->join(); } }

    // Cancel all threads
    virtual void cancel() { for (auto thr : thrs_) { thr->cancel(); } }

    // Specific one of the threads
    Runnable *operator[](int i) {  return thrs_[i].get(); }

    // Thread pool size
    int size() { return thrs_.size(); }

protected:
    std::vector<std::shared_ptr<Runnable> > thrs_;
};


typedef void(*task_func)(void *);

struct tp_task final {

    // Function pointer
    task_func func;

    // Function arguments
    void *args;

    tp_task(task_func f, void *a) : func(f), args(a) {}
};

namespace tpq {

class default_runnable;

class tp_task_queue {
    friend class default_runnable;
public:
    tp_task_queue() : stop_(false) {}

    virtual ~tp_task_queue() {}

    void add_task(std::shared_ptr<tp_task> t) {
        {
            std::unique_lock<std::mutex> lock(lock_);
            tq_.push(t);
        }
        cond_.notify_one();
    }

    void add_task(std::vector<std::shared_ptr<tp_task> > &vt) {
        {
            std::unique_lock<std::mutex> lock(lock_);
            for (auto &t : vt) { tq_.push(t); }
        }
        cond_.notify_all();
    }

protected:
    std::queue<std::shared_ptr<tp_task> > tq_;

    std::mutex lock_;

    std::condition_variable cond_;

    bool stop_;
};

class default_runnable final : public runnable {
public:
    default_runnable(tp_task_queue *tq) : tq_(tq){}

    void run_impl() override {
        while(true) {
            std::shared_ptr<tp_task> task;
            {
                std::unique_lock<std::mutex> lock(tq_->lock_);
                if (tq_->tq_.empty()) {
                    if (tq_->stop_) { break; }
                    tq_->cond_.wait(lock, [this]()->bool {
                        return tq_->tq_.size() || tq_->stop_;
                    });
                    if (tq_->tq_.empty() && tq_->stop_) { break; }
                }
                task = tq_->tq_.front();
                tq_->tq_.pop();
            }
            tq_->cond_.notify_all();
            task->func(task->args);
        }
    }

private:
    tp_task_queue *tq_;
};

class thread_pool_queue final
: public tp_task_queue,
    public thread_pool<default_runnable, tp_task_queue *>
{
public:
    thread_pool_queue(int thr_num) : tp_task_queue(),
        thread_pool<default_runnable, tp_task_queue *>(thr_num, this)
    {}

    void stop() {
        {
            std::unique_lock<std::mutex> lock(lock_);
            stop_ = true;
        }
        cond_.notify_all();
        join();
    }
};

}

using thread_pool_queue = tpq::thread_pool_queue;

}

#endif