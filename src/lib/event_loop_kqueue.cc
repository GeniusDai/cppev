#ifdef __APPLE__

namespace cppev {

static uint32_t fd_map_to_sys(fd_event ev) {
    int flags = 0;
    if (static_cast<bool>(ev & fd_event::fd_readable)) { flags |= EPOLLIN; }
    if (static_cast<bool>(ev & fd_event::fd_writable)) { flags |= EPOLLOUT; }
    return flags;
}

static fd_event fd_map_to_event(uint32_t ev) {
    fd_event flags = static_cast<fd_event>(0);
    if (ev & EPOLLIN) { flags = flags | fd_event::fd_readable; }
    if (ev & EPOLLOUT) { flags = flags | fd_event::fd_writable; }
    return flags;
}

event_loop::event_loop(void *data) : data_(data) {
    ev_fd_ = kqueue();
    if (ev_fd_ < 0) { throw_system_error("epoll_create error"); }
    on_loop_ = [](event_loop *) -> void {};
}



}

#endif