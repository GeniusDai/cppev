#ifndef _cppev_event_loop_h_6C0224787A17_
#define _cppev_event_loop_h_6C0224787A17_

#include <unordered_map>
#include <memory>
#include <unistd.h>
#include <queue>
#include <tuple>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "cppev/nio.h"
#include "cppev/sysconfig.h"
#include "cppev/utils.h"
#include "cppev/logger.h"

namespace cppev
{

enum class fd_event
{
    fd_readable = 1 << 0,
    fd_writable = 1 << 1,
};

fd_event operator&(fd_event lhs, fd_event rhs);

fd_event operator|(fd_event lhs, fd_event rhs);

fd_event operator^(fd_event lhs, fd_event rhs);

void operator&=(fd_event &lhs, fd_event rhs);

void operator|=(fd_event &lhs, fd_event rhs);

void operator^=(fd_event &lhs, fd_event rhs);

extern std::unordered_map<fd_event, const char *> fd_event_debug;

using fd_event_handler = std::function<void(const std::shared_ptr<nio> &)>;

struct fd_event_hash
{
    std::size_t operator()(const std::tuple<int, fd_event> &ev) const noexcept;
};

class event_loop
{
public:
    explicit event_loop(void *data = nullptr, void *owner = nullptr);

    event_loop(const event_loop &) = delete;
    event_loop &operator=(const event_loop &) = delete;
    event_loop(event_loop &&) = delete;
    event_loop &operator=(event_loop &&) = delete;

    virtual ~event_loop() noexcept;

    // External data for eventloop.
    void *data() noexcept;

    // External data for eventloop.
    const void *data() const noexcept;

    // External class owns eventloop.
    void *owner() noexcept;

    // External class owns eventloop.
    const void *owner() const noexcept;

    // Workloads of the event loop fd.
    int ev_loads() const noexcept;

    // Register fd event to event pollor but not activate in sys-io-multiplexing.
    // @param iop       nio smart pointer.
    // @param ev_type   event type.
    // @param handler   fd event handler.
    // @param prio      event priority.
    void fd_register(const std::shared_ptr<nio> &iop, fd_event ev_type,
        const fd_event_handler &handler = fd_event_handler(), priority prio = priority::p0);

    // Activate fd event.
    // @param iop       nio smart pointer.
    // @param ev_type   event type.
    void fd_activate(const std::shared_ptr<nio> &iop, fd_event ev_type);

    // Register fd event to event pollor and activate in sys-io-multiplexing.
    // @param iop       nio smart pointer.
    // @param ev_type   event type.
    // @param handler   fd event handler.
    // @param prio      event priority.
    void fd_register_and_activate(const std::shared_ptr<nio> &iop, fd_event ev_type,
        const fd_event_handler &handler = fd_event_handler(), priority prio = priority::p0);

    // Remove fd event from event pollor but not deactivate in sys-io-multiplexing.
    // @param iop           nio smart pointer.
    // @param ev_type   event type.
    void fd_remove(const std::shared_ptr<nio> &iop, fd_event ev_type);

    // Deactivate fd event.
    // @param iop       nio smart pointer.
    // @param ev_type   event type.
    void fd_deactivate(const std::shared_ptr<nio> &iop, fd_event ev_type);

    // Remove fd event from event pollor and deactivate in sys-io-multiplexing.
    // @param iop           nio smart pointer.
    // @param ev_type   event type.
    void fd_remove_and_deactivate(const std::shared_ptr<nio> &iop, fd_event ev_type);

    // Delete all event of the fd.
    // @param iop           nio smart pointer.
    void fd_remove_and_deactivate_all(const std::shared_ptr<nio> &iop);

    // Wait for events, only loop once, timeout unit is millisecond.
    void loop_once(int timeout = -1);

    // Stop loop once.
    void stop_loop_once();

    // Wait for events, loop infinitely, timeout unit is millisecond.
    void loop_forever(int timeout = -1);

    // Stop loop infinitely.
    void stop_loop_forever();

private:
    // Helper function to register fd event to event pollor.
    // @param iop       nio smart pointer.
    // @param ev_type   event type.
    // @param handler   fd event handler.
    // @param prio      event priority.
    void fd_register_nts(const std::shared_ptr<nio> &iop, fd_event ev_type,
        const fd_event_handler &handler, priority prio);

    // Helper function to remove fd event from event pollor.
    // @param iop           nio smart pointer.
    void fd_remove_nts(const std::shared_ptr<nio> &iop, fd_event ev_type);

    // Helper function to add fd event listening, implementation specific.
    // @param iop       nio smart pointer.
    // @param ev_type   event type.
    void fd_io_multiplexing_add_nts(const std::shared_ptr<nio> &iop, fd_event ev_type);

    // Helper function to delete fd event listening, implementation specific.
    // @param iop       nio smart pointer.
    // @param ev_type   event type.
    void fd_io_multiplexing_del_nts(const std::shared_ptr<nio> &iop, fd_event ev_type);

    // Helper function to wait for event(s) trigger, implementation specific.
    // @param timeout   timeout in millisecond, -1 means infinite.
    // @return          list of fd with an event, events of one fd are seperated.
    std::vector<std::tuple<int, fd_event>> fd_io_multiplexing_wait_ts(int timeout);

    // Protect the internal data structures to guarantee thread safety of "register / remove / loop".
    std::mutex lock_;

    // For thread sychronization in stopping loop. One possible way is using blocking io, but author has
    // witnessed read a block io in osx causing cpu 100%.
    std::condition_variable cond_;

    // Event watcher fd.
    int ev_fd_;

    // External data for eventloop.
    void *data_;

    // External class which owns eventloop.
    void *owner_;

    // Hash:   (fd, event) --> (priority, nio, callback).
    std::unordered_map<
        std::tuple<int, fd_event>,
        std::tuple<priority, std::shared_ptr<nio>, std::shared_ptr<fd_event_handler>>,
        fd_event_hash
    > fd_event_datas_;

    // Hash:   fd --> fd_event
    std::unordered_map<int, fd_event> fd_event_masks_;

    // Whether loop shall be stopped.
    bool stop_;
};

}   // namespace cppev

#endif  // event_loop.h
