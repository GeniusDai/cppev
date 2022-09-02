#include "cppev/linux_specific.h"

#ifdef __linux__

namespace cppev
{

namespace nio_factory
{

std::shared_ptr<nwatcher> get_nwatcher()
{
    int fd = inotify_init();
    if (fd < 0)
    {
        throw_system_error("inotify_init error");
    }
    return std::make_shared<nwatcher>(fd);
}

}   // namespace nio_factory

void nwatcher::add_watch(const std::string &path, uint32_t events)
{
    int wd = inotify_add_watch(fd_, path.c_str(), events);
    if (wd < 0)
    {
        throw_system_error("inotify_add_watch error");
    }
    if (wds_.count(wd) == 0)
    {
        wds_[wd] = path; paths_[path] = wd;
    }
}

void nwatcher::del_watch(const std::string &path)
{
    int wd = paths_[path];
    paths_.erase(path); wds_.erase(wd);
    if (inotify_rm_watch(fd_, paths_[path]) < 0)
    {
        throw_system_error("inotify_rm_watch error");
    }
}

void nwatcher::process_events()
{
    int len = read_chunk(sysconfig::inotify_step);
    char *p = rbuf()->rawbuf();
    for (char *s = p; s < p + len; ++s)
    {
        inotify_event *evp = (inotify_event *)s;
        handler_(evp, wds_[evp->wd].c_str());
        p += sizeof(inotify_event) + evp->len;
    }
}

timer::timer(int interval, const timer_handler &handler)
: handler_(handler)
{
    sigevent sev;
    memset(&sev, 0, sizeof(sev));
    sev.sigev_value.sival_ptr = this;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = [](sigval value) -> void
    {
        timer *pseudo_this = reinterpret_cast<timer *>(value.sival_ptr);
        pseudo_this->handler_();
    };
    if (timer_create(CLOCK_REALTIME, &sev, &tmid_) != 0)
    {
        throw_system_error("timer_create error");
    }
    itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1;
    its.it_interval.tv_sec = interval / 1000;
    its.it_interval.tv_nsec = (interval % 1000) * 1000 * 1000;
    if (timer_settime(tmid_, 0, &its, nullptr) != 0)
    {
        throw_system_error("timer_settime error");
    }
}

void timer::stop()
{
    if (timer_delete(tmid_) != 0)
    {
        throw_system_error("timer_delete error");
    }
}

}   // namespace cppev

#endif  // __linux__
