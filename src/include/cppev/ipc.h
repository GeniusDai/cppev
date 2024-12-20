#ifndef _ipc_h_6C0224787A17_
#define _ipc_h_6C0224787A17_

#include "cppev/utils.h"
#include <mutex>
#include <condition_variable>
#include <string>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>

namespace cppev
{

class shared_memory final
{
public:
    shared_memory(const std::string &name, int size, mode_t mode = 0600);

    shared_memory(const shared_memory &) = delete;
    shared_memory &operator=(const shared_memory &) = delete;

    shared_memory(shared_memory &&other) noexcept
    {
        if (&other == this)
        {
            return;
        }
        move(std::forward<shared_memory>(other));
    }

    shared_memory &operator=(shared_memory &&other) noexcept
    {
        if (&other == this)
        {
            return *this;
        }
        move(std::forward<shared_memory>(other));
        return *this;
    }

    ~shared_memory() noexcept;

    template <typename SharedClass, typename... Args>
    SharedClass *construct(Args&&... args)
    {
        SharedClass *object = new (ptr_) SharedClass(std::forward<Args>(args)...);
        if (object == nullptr)
        {
            throw_runtime_error("placement new error");
        }
        return object;
    }

    void unlink();

    void *ptr() const noexcept
    {
        return ptr_;
    }

    int size() const noexcept
    {
        return size_;
    }

    bool creator() const noexcept
    {
        return creator_;
    }

private:
    void move(shared_memory &&other) noexcept
    {
        this->name_ = other.name_;
        this->size_ = other.size_;
        this->ptr_ = other.ptr_;
        this->creator_ = other.creator_;

        other.name_ = "";
        other.size_ = 0;
        other.ptr_ = nullptr;
        other.creator_ = false;
    }

    std::string name_;

    int size_;

    void *ptr_;

    bool creator_;
};

class semaphore final
{
public:
    explicit semaphore(const std::string &name, mode_t mode = 0600);

    semaphore(const semaphore &) = delete;
    semaphore &operator=(const semaphore &) = delete;

    semaphore(semaphore &&other) noexcept
    {
        if (&other == this)
        {
            return;
        }
        move(std::forward<semaphore>(other));
    }

    semaphore &operator=(semaphore &&other) noexcept
    {
        if (&other == this)
        {
            return *this;
        }
        move(std::forward<semaphore>(other));
        return *this;
    }

    ~semaphore() noexcept;

    bool try_acquire();

    void acquire(int count = 1);

    void release(int count = 1);

    void unlink();

    bool creator() const noexcept
    {
        return creator_;
    }

private:
    void move(semaphore &&other) noexcept
    {
        this->name_ = other.name_;
        this->sem_ = other.sem_;
        this->creator_ = other.creator_;

        other.name_ = "";
        other.sem_ = SEM_FAILED;
        other.creator_ = false;
    }

    std::string name_;

    sem_t *sem_;

    bool creator_;
};

class pshared_lock final
{
    friend class pshared_cond;
public:
    pshared_lock();

    pshared_lock(const pshared_lock &) = delete;
    pshared_lock &operator=(const pshared_lock &) = delete;
    pshared_lock(pshared_lock &&) = delete;
    pshared_lock &operator=(pshared_lock &&) = delete;

    ~pshared_lock() noexcept;

    void lock();

    bool try_lock();

    void unlock();

private:
    pthread_mutex_t lock_;
};

class pshared_cond final
{
public:
    using predicate = std::function<bool()>;

    pshared_cond();

    pshared_cond(const pshared_cond &) = delete;
    pshared_cond &operator=(const pshared_cond &) = delete;
    pshared_cond(pshared_cond &&) = delete;
    pshared_cond &operator=(pshared_cond &&) = delete;

    ~pshared_cond() noexcept;

    void wait(std::unique_lock<pshared_lock> &lock);

    void wait(std::unique_lock<pshared_lock> &lock, const predicate &pred);

    template<class Rep, class Period>
    std::cv_status wait_for(
        std::unique_lock<pshared_lock> &lock,
        const std::chrono::duration<Rep, Period> &rel_time
    )
    {
        // The implementation uses system clock to align with the standard library.
        auto n_rel_time = std::chrono::duration_cast<std::chrono::nanoseconds>(rel_time).count();
        timeval tv;
        int ret = gettimeofday(&tv, nullptr);
        if (ret != 0)
        {
            throw_system_error("gettimeofday error");
        }
        auto n_abs_time = n_rel_time + tv.tv_sec * 1'000'000'000 + tv.tv_usec * 1'000;
        timespec ts;
        ts.tv_sec = n_abs_time / 1'000'000'000;
        ts.tv_nsec = n_abs_time % 1'000'000'000;
        ret = pthread_cond_timedwait(&cond_, &lock.mutex()->lock_, &ts);
        std::cv_status status = std::cv_status::no_timeout;
        if (ret != 0)
        {
            if (ret == EINVAL)
            {
                throw_system_error("pthread_cond_wait error", ret);
            }
            else if (ret == ETIMEDOUT)
            {
                status = std::cv_status::timeout;
            }
        }
        return status;
    }

    template<class Rep, class Period>
    bool wait_for(
        std::unique_lock<pshared_lock> &lock,
        const std::chrono::duration<Rep, Period> &rel_time,
        const predicate &pred
    )
    {
        wait_for(lock, rel_time);
        return pred();
    }

    void notify_one();

    void notify_all();

private:
    pthread_cond_t cond_;
};

class pshared_one_time_fence final
{
public:
    pshared_one_time_fence();

    pshared_one_time_fence(const pshared_one_time_fence &) = delete;
    pshared_one_time_fence &operator=(const pshared_one_time_fence &) = delete;
    pshared_one_time_fence(pshared_one_time_fence &&) = delete;
    pshared_one_time_fence &operator=(pshared_one_time_fence &&) = delete;

    ~pshared_one_time_fence() = default;

    void wait();

    void notify();

    bool ok() const noexcept;

private:
    bool ok_;

    pshared_lock lock_;

    pshared_cond cond_;
};

class pshared_barrier final
{
public:
    pshared_barrier(int count);

    pshared_barrier(const pshared_barrier &) = delete;
    pshared_barrier &operator=(const pshared_barrier &) = delete;
    pshared_barrier(pshared_barrier &&) = delete;
    pshared_barrier &operator=(pshared_barrier &&) = delete;

    ~pshared_barrier() = default;

    void wait();

private:
    int count_;

    pshared_lock lock_;

    pshared_cond cond_;
};

class pshared_rwlock final
{
public:
    pshared_rwlock();

    pshared_rwlock(const pshared_rwlock &) = delete;
    pshared_rwlock &operator=(const pshared_rwlock &) = delete;
    pshared_rwlock(pshared_rwlock &&) = delete;
    pshared_rwlock &operator=(pshared_rwlock &&) = delete;

    ~pshared_rwlock() noexcept;

    void unlock();

    void rdlock();

    void wrlock();

    bool try_rdlock();

    bool try_wrlock();

private:
    pthread_rwlock_t lock_;
};

class rdlockguard final
{
public:
    explicit rdlockguard(pshared_rwlock &lock);

    rdlockguard(const rdlockguard &) = delete;
    rdlockguard &operator=(const rdlockguard &) = delete;
    rdlockguard(rdlockguard &&other) noexcept;
    rdlockguard &operator=(rdlockguard &&other) noexcept;

    ~rdlockguard() noexcept;

    void lock();

    void unlock();

private:
    pshared_rwlock *rwlock_;
};

class wrlockguard final
{
public:
    explicit wrlockguard(pshared_rwlock &lock);

    wrlockguard(const wrlockguard &) = delete;
    wrlockguard &operator=(const wrlockguard &) = delete;
    wrlockguard(wrlockguard &&other) noexcept;
    wrlockguard &operator=(wrlockguard &&other) noexcept;

    ~wrlockguard() noexcept;

    void lock();

    void unlock();

private:
    pshared_rwlock *rwlock_;
};

}   // namespace cppev

#endif  // ipc.h
