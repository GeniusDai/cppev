#ifndef _event_loop_h_6C0224787A17_
#define _event_loop_h_6C0224787A17_

#include <unordered_map>
#include <memory>
#include <unistd.h>
#include <queue>
#include <tuple>
#include <mutex>
#include "cppev/nio.h"
#include "cppev/sysconfig.h"
#include "cppev/common_utils.h"

namespace cppev {

enum class fd_event {
    fd_readable = 1 << 0,
    fd_writable = 1 << 1,
};

constexpr fd_event operator&(fd_event a, fd_event b) {
    return static_cast<fd_event>(static_cast<int>(a) & static_cast<int>(b));
}

constexpr fd_event operator|(fd_event a, fd_event b) {
    return static_cast<fd_event>(static_cast<int>(a) | static_cast<int>(b));
}

enum priority {
    low = 10,
    high = 20
};

class event_loop;

typedef void (*ev_handler)(event_loop *);

typedef void (*fd_event_cb)(std::shared_ptr<nio> iop);

class event_loop : public uncopyable {
public:
    int ev_fd() const { return ev_fd_; }

    void *data() { return data_; }

    event_loop(void *data = nullptr);

    virtual ~event_loop() { close(ev_fd_); }

    // measure the loads of event loop
    int fd_loads() const { return fds_.size(); }

    void set_on_loop(ev_handler h) { on_loop_ = h; }

    // register fd to event pollor, default activate callback
    void fd_register(std::shared_ptr<nio> iop, fd_event ev_type,
        fd_event_cb ev_cb = nullptr, bool activate = true, priority prio = low);

    // remove fd from event pollor, default clean callback
    void fd_remove(std::shared_ptr<nio> iop, bool clean = true);

    // wait for events, only loop once, timeout unit is millisecond
    void loop_once(int timeout = -1);

    void loop(int timeout = -1)
    {
        while(true) {
            on_loop_(this);
            loop_once();
        }
    }

private:
    // when event loop starts;
    ev_handler on_loop_;

    // when register callback for event loop
    std::mutex lock_;

    // event watcher fd
    int ev_fd_;

    // thread pool shared data, may be used for callbacks
    void *data_;

    // <priority, nio, callback, event>
    std::unordered_multimap<int, std::tuple<int, std::shared_ptr<nio>, fd_event_cb, fd_event> > fds_;

    // <priority, nio, callback>
    std::priority_queue<std::tuple<int, std::shared_ptr<nio>, fd_event_cb> > fd_events_;
};

}

#endif