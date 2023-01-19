#ifndef _ipc_h_6C0224787A17_
#define _ipc_h_6C0224787A17_

#include "cppev/utils.h"
#include <string>
#include <semaphore.h>

namespace cppev
{

class shared_memory final
{
public:
    shared_memory(const std::string &name, int size, mode_t mode = 0600);

    shared_memory(const shared_memory &) = delete;
    shared_memory &operator=(const shared_memory &) = delete;
    shared_memory(shared_memory &&) = delete;
    shared_memory &operator=(shared_memory &&) = delete;

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
    std::string name_;

    int size_;

    void *ptr_;

    bool creator_;
};

class semaphore final
{
public:
    semaphore(const std::string &name, mode_t mode = 0600);

    semaphore(const semaphore &) = delete;
    semaphore &operator=(const semaphore &) = delete;
    semaphore(semaphore &&) = delete;
    semaphore &operator=(semaphore &&) = delete;

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
    std::string name_;

    sem_t *sem_;

    bool creator_;
};

}   // namespace cppev

#endif  // ipc.h