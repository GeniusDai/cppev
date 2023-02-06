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

class subp_open final
{
public:
    explicit subp_open(const std::string &cmd, char *const *envp=nullptr);

    subp_open(const subp_open &) = delete;
    subp_open &operator=(const subp_open &) = delete;

    subp_open(subp_open &&other) noexcept
    {
        move(std::forward<subp_open>(other));
    }

    subp_open &operator=(subp_open &&other) noexcept
    {
        move(std::forward<subp_open>(other));
        return *this;
    }

    ~subp_open() = default;

    bool poll();

    template <typename Rep, typename Period>
    void wait(const std::chrono::duration<Rep, Period> &interval);

    void wait()
    {
        wait(std::chrono::milliseconds(50));
    }

    void communicate(const char *input, int len);

    void communicate()
    {
        communicate(nullptr, 0);
    }

    void communicate(const std::string &input)
    {
        communicate(input.c_str(), input.size());
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
        return stdout_->rbuffer().rawbuf();
    }

    const char *stderr() const noexcept
    {
        return stderr_->rbuffer().rawbuf();
    }

    pid_t pid() const noexcept
    {
        return pid_;
    }

private:
    void move(subp_open &&other) noexcept
    {
        this->cmd_ = other.cmd_;
        this->envp_ = other.envp_;
        this->stdin_ = std::move(other.stdin_);
        this->stdout_ = std::move(other.stdout_);
        this->stderr_ = std::move(other.stderr_);
        this->pid_ = other.pid_;
        this->returncode_ = other.returncode_;

        other.cmd_ = "";
        other.envp_ = nullptr;
        other.pid_ = 0;
        other.returncode_ = -1;
    }

    std::string cmd_;

    char *const *envp_;

    std::unique_ptr<nstream> stdin_;

    std::unique_ptr<nstream> stdout_;

    std::unique_ptr<nstream> stderr_;

    pid_t pid_;

    int returncode_;
};

template<typename Rep, typename Period>
void subp_open::wait(const std::chrono::duration<Rep, Period> &interval)
{
    /*
     * Q : Why polling is essential ?
     * A : Buffer size of pipe is limited, so if you just wait for the subprocess terminates,
     *     subprocess may block at writing to stdout or stderr. So that means you need to
     *     simultaneously deal with the io and query the subprocess termination. Of course you
     *     can use SIGCHLD but this will make the program too complicated.
     */
    while (!poll())
    {
        communicate();
        std::this_thread::sleep_for(interval);
    }
    communicate();
}

}   // namespace cppev

#endif  // subprocess.h
