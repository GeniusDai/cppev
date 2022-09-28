#ifndef _subprocess_h_6C0224787A17_
#define _subprocess_h_6C0224787A17_

#include <vector>
#include <string>
#include <tuple>
#include <memory>
#include <chrono>
#include "cppev/nio.h"

namespace cppev
{

namespace subprocess
{

std::tuple<int, std::string, std::string> exec_cmd(const char *cmd, char *const *envp = nullptr);

std::tuple<int, std::string, std::string> exec_cmd(const std::string &cmd, char *const *envp = nullptr);

}   // namespace subprocess

class popen final
{
public:
    popen(const std::string &cmd, char *const *envp=nullptr);

    popen(const popen &) = delete;
    popen &operator=(const popen &) = delete;
    popen(popen &&) = delete;
    popen &operator=(popen &&) = delete;

    ~popen() = default;

    bool poll();

    template <typename Rep, typename Period>
    void wait(const std::chrono::duration<Rep, Period> &interval);

    void wait()
    {
        wait(std::chrono::milliseconds(50));
    }

    void communicate(const std::string &str="");

    void terminate();

    void kill();

    void send_signal(int sig);

    int returncode() const
    {
        return returncode_;
    }

    const char *stdout() const
    {
        return stdout_->rbuf()->rawbuf();
    }

    const char *stderr() const
    {
        return stderr_->rbuf()->rawbuf();
    }

    pid_t pid() const
    {
        return pid_;
    }

private:
    std::string cmd_;

    char *const *envp_;

    std::shared_ptr<nstream> stdin_;

    std::shared_ptr<nstream> stdout_;

    std::shared_ptr<nstream> stderr_;

    pid_t pid_;

    int returncode_;
};

template<typename Rep, typename Period>
void popen::wait(const std::chrono::duration<Rep, Period> &interval)
{
    while (!poll())
    {
        communicate();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    communicate();
}

}   // namespace cppev

#endif  // subprocess.h
