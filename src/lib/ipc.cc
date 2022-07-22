#include "cppev/ipc.h"
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

namespace cppev
{

shared_memory::shared_memory(const std::string &name, int size, bool create, mode_t mode)
: name_(name), size_(size)
{
    int flag = O_RDWR;
    if (create)
    {
        flag |= O_CREAT | O_EXCL;
    }
    fd_ = shm_open(name_.c_str(), flag, mode);
    if (fd_ < 0)
    {
        throw_system_error("shm_open error");
    }
    if (create)
    {
        ftruncate(fd_, size_);
    }
    ptr_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (ptr_ == MAP_FAILED)
    {
        throw_system_error("mmap error");
    }
    if (create)
    {
        memset(ptr_, 0, size_);
    }
}

shared_memory::~shared_memory()
{
    close(fd_);
}

void shared_memory::unlink()
{
    if (shm_unlink(name_.c_str()) == -1)
    {
        throw_system_error("shm_unlink error");
    }
}



semaphore::semaphore(const std::string &name, int value, mode_t mode)
: name_(name)
{
    if (value != -1)
    {
        sem_ = sem_open(name_.c_str(), O_CREAT | O_EXCL, mode, value);
    }
    else
    {
        sem_ = sem_open(name_.c_str(), 0);
    }
    if (sem_ == SEM_FAILED)
    {
        throw_system_error("sem_open error");
    }
}

semaphore::~semaphore()
{
    if (sem_close(sem_) == -1)
    {
        throw_system_error("sem_close error");
    }
}

void semaphore::unlink()
{
    if (sem_unlink(name_.c_str()) == -1)
    {
        throw_system_error("sem_unlink error");
    }
}

bool semaphore::acquire(int timeout)
{
    int ret;
    if (timeout == -1)
    {
        ret = sem_wait(sem_);
    }
    else if (timeout == 0)
    {
        ret = sem_trywait(sem_);
    }
    if (ret == -1)
    {
        if (errno == EAGAIN || errno == EINTR)
        {
            return false;
        }
        std::string str = timeout == -1 ? "sem_wait" : "sem_trywait";
        throw_system_error(str.append(" error"));
    }
    return true;
}

void semaphore::release()
{
    if (sem_post(sem_) == -1)
    {
        throw_system_error("sem_post error");
    }
}

}   // namespace cppev
