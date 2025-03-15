#ifndef _cppev_ipc_h_6C0224787A17_
#define _cppev_ipc_h_6C0224787A17_

#include <semaphore.h>
#include <sys/time.h>

#include <string>

#include "cppev/common.h"
#include "cppev/utils.h"

namespace cppev
{

class CPPEV_PUBLIC shared_memory final
{
public:
    shared_memory(const std::string &name, int size, mode_t mode = 0600);

    shared_memory(const shared_memory &) = delete;
    shared_memory &operator=(const shared_memory &) = delete;
    shared_memory(shared_memory &&other) noexcept;
    shared_memory &operator=(shared_memory &&other) noexcept;

    ~shared_memory() noexcept;

    template <typename SharedClass, typename... Args>
    SharedClass *construct(Args &&...args)
    {
        SharedClass *object =
            new (ptr_) SharedClass(std::forward<Args>(args)...);
        if (object == nullptr)
        {
            throw_runtime_error("placement new error");
        }
        return object;
    }

    void unlink();

    void *ptr() const noexcept;

    int size() const noexcept;

    bool creator() const noexcept;

private:
    void move(shared_memory &&other) noexcept;

    std::string name_;

    int size_;

    void *ptr_;

    bool creator_;
};

class CPPEV_PUBLIC semaphore final
{
public:
    explicit semaphore(const std::string &name, mode_t mode = 0600);

    semaphore(const semaphore &) = delete;
    semaphore &operator=(const semaphore &) = delete;
    semaphore(semaphore &&other) noexcept;
    semaphore &operator=(semaphore &&other) noexcept;

    ~semaphore() noexcept;

    bool try_acquire();

    void acquire(int count = 1);

    void release(int count = 1);

    void unlink();

    bool creator() const noexcept;

private:
    void move(semaphore &&other) noexcept;

    std::string name_;

    sem_t *sem_;

    bool creator_;
};

}  // namespace cppev

#endif  // ipc.h
