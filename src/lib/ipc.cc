#include "cppev/ipc.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>

namespace cppev
{

shared_memory::shared_memory(const std::string &name, int size, mode_t mode)
    : name_(name), size_(size), ptr_(nullptr), creator_(false)
{
    int fd = shm_open(name_.c_str(), O_RDWR, mode);
    if (fd < 0)
    {
        if (errno == ENOENT)
        {
            fd = shm_open(name_.c_str(), O_RDWR | O_CREAT | O_EXCL, mode);
            if (fd < 0)
            {
                if (errno == EEXIST)
                {
                    fd = shm_open(name_.c_str(), O_RDWR, mode);
                    if (fd < 0)
                    {
                        throw_system_error("shm_open error");
                    }
                }
                else
                {
                    throw_system_error("shm_open error");
                }
            }
            else
            {
                creator_ = true;
            }
        }
        else
        {
            throw_system_error("shm_open error");
        }
    }

    if (creator_)
    {
        int ret = ftruncate(fd, size_);
        if (ret == -1)
        {
            throw_system_error("ftruncate error");
        }
    }
    ptr_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr_ == MAP_FAILED)
    {
        throw_system_error("mmap error");
    }
    close(fd);
    if (creator_)
    {
        memset(ptr_, 0, size_);
    }
}

shared_memory::shared_memory(shared_memory &&other) noexcept
{
    if (&other == this)
    {
        return;
    }
    move(std::forward<shared_memory>(other));
}

shared_memory &shared_memory::operator=(shared_memory &&other) noexcept
{
    if (&other == this)
    {
        return *this;
    }
    move(std::forward<shared_memory>(other));
    return *this;
}

shared_memory::~shared_memory() noexcept
{
    if (ptr_ != nullptr && size_ != 0)
    {
        munmap(ptr_, size_);
    }
}

void shared_memory::unlink()
{
    if (name_.size() && (shm_unlink(name_.c_str()) == -1))
    {
        throw_system_error("shm_unlink error");
    }
}

void *shared_memory::ptr() const noexcept
{
    return ptr_;
}

int shared_memory::size() const noexcept
{
    return size_;
}

bool shared_memory::creator() const noexcept
{
    return creator_;
}

void shared_memory::move(shared_memory &&other) noexcept
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

semaphore::semaphore(const std::string &name, mode_t mode)
    : name_(name), sem_(nullptr), creator_(false)
{
    sem_ = sem_open(name_.c_str(), 0);
    if (sem_ == SEM_FAILED)
    {
        if (errno == ENOENT)
        {
            sem_ = sem_open(name_.c_str(), O_CREAT | O_EXCL, mode, 0);
            if (sem_ == SEM_FAILED)
            {
                if (errno == EEXIST)
                {
                    sem_ = sem_open(name_.c_str(), 0);
                    if (sem_ == SEM_FAILED)
                    {
                        throw_system_error("sem_open error");
                    }
                }
                else
                {
                    throw_system_error("sem_open error");
                }
            }
            else
            {
                creator_ = true;
            }
        }
        else
        {
            throw_system_error("sem_open error");
        }
    }
}

semaphore::semaphore(semaphore &&other) noexcept
{
    if (&other == this)
    {
        return;
    }
    move(std::forward<semaphore>(other));
}

semaphore &semaphore::operator=(semaphore &&other) noexcept
{
    if (&other == this)
    {
        return *this;
    }
    move(std::forward<semaphore>(other));
    return *this;
}

semaphore::~semaphore() noexcept
{
    if (sem_ != SEM_FAILED)
    {
        sem_close(sem_);
    }
}

bool semaphore::try_acquire()
{
    if (sem_trywait(sem_) == -1)
    {
        if (errno == EINTR || errno == EAGAIN)
        {
            return false;
        }
        else
        {
            throw_system_error("sem_trywait error");
        }
    }
    return true;
}

void semaphore::acquire(int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (sem_wait(sem_) == -1)
        {
            throw_system_error("sem_wait error");
        }
    }
}

void semaphore::release(int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (sem_post(sem_) == -1)
        {
            throw_system_error("sem_post error");
        }
    }
}

void semaphore::unlink()
{
    if (name_.size() && (sem_unlink(name_.c_str()) == -1))
    {
        throw_system_error("sem_unlink error");
    }
}

bool semaphore::creator() const noexcept
{
    return creator_;
}

void semaphore::move(semaphore &&other) noexcept
{
    this->name_ = other.name_;
    this->sem_ = other.sem_;
    this->creator_ = other.creator_;

    other.name_ = "";
    other.sem_ = SEM_FAILED;
    other.creator_ = false;
}

pshared_lock::pshared_lock()
{
    int ret = 0;
    pthread_mutexattr_t attr;
    ret = pthread_mutexattr_init(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_mutexattr_init error",
                                               ret);
    }
    ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_mutexattr_settype error", ret);
    }
    ret = pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_mutexattr_setprotocol error", ret);
    }
    ret = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_mutexattr_setpshared error", ret);
    }
    ret = pthread_mutex_init(&lock_, &attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_mutex_init error", ret);
    }
    ret = pthread_mutexattr_destroy(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_mutexattr_destroy error", ret);
    }
}

pshared_lock::~pshared_lock() noexcept
{
    pthread_mutex_destroy(&lock_);
}

void pshared_lock::lock()
{
    int ret = pthread_mutex_lock(&lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_mutex_lock error", ret);
    }
}

bool pshared_lock::try_lock()
{
    int ret = pthread_mutex_trylock(&lock_);
    if (ret != 0)
    {
        if (ret == EBUSY)
        {
            return false;
        }
        throw_system_error_with_specific_errno("pthread_mutex_trylock error",
                                               ret);
    }
    return true;
}

void pshared_lock::unlock()
{
    int ret = pthread_mutex_unlock(&lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_mutex_unlock error",
                                               ret);
    }
}

pshared_cond::pshared_cond()
{
    int ret = 0;
    pthread_condattr_t attr;
    ret = pthread_condattr_init(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_condattr_init error",
                                               ret);
    }
    ret = pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_condattr_setpshared error", ret);
    }
    ret = pthread_cond_init(&cond_, &attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_cond_init error", ret);
    }
    ret = pthread_condattr_destroy(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_condattr_destroy error",
                                               ret);
    }
}

pshared_cond::~pshared_cond() noexcept
{
    pthread_cond_destroy(&cond_);
}

void pshared_cond::wait(std::unique_lock<pshared_lock> &lock)
{
    int ret = pthread_cond_wait(&cond_, &lock.mutex()->lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_cond_wait error", ret);
    }
}

void pshared_cond::wait(std::unique_lock<pshared_lock> &lock,
                        const predicate &pred)
{
    while (!pred())
    {
        wait(lock);
    }
}

void pshared_cond::notify_one()
{
    int ret = pthread_cond_signal(&cond_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_cond_signal error",
                                               ret);
    }
}

void pshared_cond::notify_all()
{
    int ret = pthread_cond_broadcast(&cond_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_cond_broadcast error",
                                               ret);
    }
}

pshared_one_time_fence::pshared_one_time_fence() : ok_(false)
{
}

pshared_one_time_fence::~pshared_one_time_fence() = default;

void pshared_one_time_fence::wait()
{
    if (!ok_)
    {
        std::unique_lock<pshared_lock> lock(lock_);
        if (!ok_)
        {
            cond_.wait(lock,
                       [this]()
                       {
                           return ok_;
                       });
        }
    }
}

void pshared_one_time_fence::notify()
{
    if (!ok_)
    {
        std::unique_lock<pshared_lock> lock(lock_);
        ok_ = true;
        cond_.notify_one();
    }
}

bool pshared_one_time_fence::ok() const noexcept
{
    return ok_;
}

pshared_barrier::pshared_barrier(int count) : count_(count)
{
}

pshared_barrier::~pshared_barrier() = default;

void pshared_barrier::wait()
{
    std::unique_lock<pshared_lock> lock(lock_);
    --count_;
    if (count_ == 0)
    {
        cond_.notify_all();
    }
    else if (count_ > 0)
    {
        cond_.wait(lock,
                   [this]()
                   {
                       return count_ == 0;
                   });
    }
    else
    {
        throw_logic_error("too many threads waited in the barrier");
    }
}

pshared_rwlock::pshared_rwlock()
{
    int ret = 0;
    pthread_rwlockattr_t attr;
    ret = pthread_rwlockattr_init(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_rwlockattr_init error",
                                               ret);
    }
    ret = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_rwlockattr_setpshared error", ret);
    }
    ret = pthread_rwlock_init(&lock_, &attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_rwlock_init error",
                                               ret);
    }
    ret = pthread_rwlockattr_destroy(&attr);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno(
            "pthread_rwlockattr_destroy error", ret);
    }
}

pshared_rwlock::~pshared_rwlock() noexcept
{
    pthread_rwlock_destroy(&lock_);
}

void pshared_rwlock::unlock()
{
    int ret = pthread_rwlock_unlock(&lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_rwlock_unlock error",
                                               ret);
    }
}

void pshared_rwlock::rdlock()
{
    int ret = pthread_rwlock_rdlock(&lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_rwlock_rdlock error",
                                               ret);
    }
}

void pshared_rwlock::wrlock()
{
    int ret = pthread_rwlock_wrlock(&lock_);
    if (ret != 0)
    {
        throw_system_error_with_specific_errno("pthread_rwlock_wrlock error",
                                               ret);
    }
}

bool pshared_rwlock::try_rdlock()
{
    int ret = pthread_rwlock_tryrdlock(&lock_);
    if (ret == 0)
    {
        return true;
    }
    else if (ret == EBUSY || ret == EAGAIN)
    {
        return false;
    }
    throw_system_error_with_specific_errno("pthread_rwlock_tryrdlock error",
                                           ret);
    return ret;
}

bool pshared_rwlock::try_wrlock()
{
    int ret = pthread_rwlock_trywrlock(&lock_);
    if (ret == 0)
    {
        return true;
    }
    else if (ret == EBUSY)
    {
        return false;
    }
    throw_system_error_with_specific_errno("pthread_rwlock_trywrlock error",
                                           ret);
    return ret;
}

rdlockguard::rdlockguard(pshared_rwlock &lock) : rwlock_(&lock)
{
    rwlock_->rdlock();
}

rdlockguard::rdlockguard(rdlockguard &&other) noexcept
{
    this->rwlock_ = other.rwlock_;
    other.rwlock_ = nullptr;
}

rdlockguard &rdlockguard::operator=(rdlockguard &&other) noexcept
{
    this->rwlock_ = other.rwlock_;
    other.rwlock_ = nullptr;

    return *this;
}

rdlockguard::~rdlockguard() noexcept
{
    if (rwlock_ != nullptr)
    {
        try
        {
            rwlock_->unlock();
        }
        catch (...)
        {
        }
    }
}

void rdlockguard::lock()
{
    rwlock_->rdlock();
}

void rdlockguard::unlock()
{
    rwlock_->unlock();
}

wrlockguard::wrlockguard(pshared_rwlock &lock) : rwlock_(&lock)
{
    rwlock_->wrlock();
}

wrlockguard::wrlockguard(wrlockguard &&other) noexcept
{
    this->rwlock_ = other.rwlock_;
    other.rwlock_ = nullptr;
}

wrlockguard &wrlockguard::operator=(wrlockguard &&other) noexcept
{
    this->rwlock_ = other.rwlock_;
    other.rwlock_ = nullptr;

    return *this;
}

wrlockguard::~wrlockguard() noexcept
{
    if (rwlock_ != nullptr)
    {
        try
        {
            rwlock_->unlock();
        }
        catch (...)
        {
        }
    }
}

void wrlockguard::lock()
{
    rwlock_->wrlock();
}

void wrlockguard::unlock()
{
    rwlock_->unlock();
}

}  // namespace cppev
