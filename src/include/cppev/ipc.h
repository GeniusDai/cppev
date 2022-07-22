#ifndef _ipc_h_6C0224787A17_
#define _ipc_h_6C0224787A17_

#include "cppev/common_utils.h"
#include <string>
#include <semaphore.h>

namespace cppev
{

class shared_memory
: public uncopyable
{
public:
    shared_memory(const std::string &name, int size, bool create, mode_t mode = 0600);

    ~shared_memory();

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

    int fd_;

    void *ptr_;

};

class semaphore
: public uncopyable
{
public:
    semaphore(const std::string &name, int value = -1, mode_t mode = 0600);

    ~semaphore();

    void unlink();

    bool acquire(int timeout = -1);

    bool try_acquire()
    {
        return acquire(0);
    }

    void release();

private:
    std::string name_;

    sem_t *sem_;
};

}   // namespace cppev

#endif  // ipc.h