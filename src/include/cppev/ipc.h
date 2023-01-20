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

    shared_memory(shared_memory &&other) noexcept
    {
        move(std::forward<shared_memory>(other));
    }

    shared_memory &operator=(shared_memory &&other) noexcept
    {
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
        move(std::forward<semaphore>(other));
    }

    semaphore &operator=(semaphore &&other) noexcept
    {
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

}   // namespace cppev

#endif  // ipc.h