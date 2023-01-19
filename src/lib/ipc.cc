#include "cppev/ipc.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

namespace cppev
{

shared_memory::shared_memory(const std::string &name, int size, mode_t mode)
: name_(name), size_(size), ptr_(nullptr), creator_(false)
{
    int fd = -1;
    fd = shm_open(name_.c_str(), O_RDWR, mode);
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
        ftruncate(fd, size_);
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

shared_memory::~shared_memory() noexcept
{
    munmap(ptr_, size_);
}

void shared_memory::unlink()
{
    if (shm_unlink(name_.c_str()) == -1)
    {
        throw_system_error("shm_unlink error");
    }
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

semaphore::~semaphore() noexcept
{
    sem_close(sem_);
}

bool semaphore::try_acquire()
{
    if(sem_trywait(sem_) == -1)
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
    if (sem_unlink(name_.c_str()) == -1)
    {
        throw_system_error("sem_unlink error");
    }
}

}   // namespace cppev
