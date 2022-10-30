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

    void communicate(const char *input = nullptr, int len = 0);

    void communicate(const std::string &input)
    {
        communicate(input.c_str());
    }

    void terminate() const;

    void kill() const;

    void send_signal(int sig) const;

    int returncode() const noexcept
    {
        return returncode_;
    }

    const char *stdout() const noexcept
    {
        return stdout_->rbuf().rawbuf();
    }

    const char *stderr() const noexcept
    {
        return stderr_->rbuf().rawbuf();
    }

    pid_t pid() const noexcept
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
        std::this_thread::sleep_for(interval);
    }
    communicate();
}

}   // namespace cppev

#endif  // subprocess.h
