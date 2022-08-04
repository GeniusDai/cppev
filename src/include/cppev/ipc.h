#ifndef _ipc_h_6C0224787A17_
#define _ipc_h_6C0224787A17_

#include "cppev/common_utils.h"
#include <string>
#include <semaphore.h>

namespace cppev
{

class shared_memory final
{
public:
    shared_memory(const std::string &name, int size, bool create, mode_t mode = 0600);

    shared_memory(const shared_memory &) = delete;
    shared_memory &operator=(const shared_memory &) = delete;
    shared_memory(shared_memory &&) = delete;
    shared_memory &operator=(shared_memory &&) = delete;

    ~shared_memory();

    template <typename SharedClass, typename... Args>
    void constructor(Args&&... args)
    {
        SharedClass *object = new (ptr_) SharedClass(std::forward<Args>(args)...);
        if (object == nullptr)
        {
            throw_runtime_error("placement new error");
        }
    }

    template <typename SharedClass>
    void destructor()
    {
        SharedClass *object = reinterpret_cast<SharedClass *>(ptr_);
        object->~SharedClass();
    }

    void unlink();

    void *ptr()
    {
        return ptr_;
    }

    int size() const
    {
        return size_;
    }

private:
    std::string name_;

    int size_;

    void *ptr_;
};

class semaphore final
{
public:
    semaphore(const std::string &name, int value = -1, mode_t mode = 0600);

    semaphore(const semaphore &) = delete;
    semaphore &operator=(const semaphore &) = delete;
    semaphore(semaphore &&) = delete;
    semaphore &operator=(semaphore &&) = delete;

    ~semaphore();

    bool acquire()
    {
        return acquire(-1);
    }

    bool try_acquire()
    {
        return acquire(0);
    }

    void release();

#if defined(__linux__)
    int getvalue();
#endif

    void unlink();

private:
    bool acquire(int timeout);

    std::string name_;

    sem_t *sem_;
};

}   // namespace cppev

#endif  // ipc.h