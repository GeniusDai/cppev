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
        this->stdin_ = other.stdin_;
        this->stdout_ = other.stdout_;
        this->stderr_ = other.stderr_;
        this->pid_ = other.pid_;
        this->returncode_ = other.returncode_;

        other.cmd_ = "";
        other.envp_ = nullptr;
        other.stdin_ = nullptr;
        other.stdout_ = nullptr;
        other.stderr_ = nullptr;
        other.pid_ = 0;
        other.returncode_ = -1;
    }

    std::string cmd_;

    char *const *envp_;

    std::shared_ptr<nstream> stdin_;

    std::shared_ptr<nstream> stdout_;

    std::shared_ptr<nstream> stderr_;

    pid_t pid_;

    int returncode_;
};

template<typename Rep, typename Period>
void subp_open::wait(const std::chrono::duration<Rep, Period> &interval)
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
