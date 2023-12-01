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

    subp_open(subp_open &&other) = default;
    subp_open &operator=(subp_open &&other) = default;

    ~subp_open() = default;

    bool poll();

    template <typename Rep, typename Period>
    void wait(const std::chrono::duration<Rep, Period> &interval)
    {
        /*
         * Q : Why polling is essential?
         * A : Buffer size of pipe is limited, so if you just wait for the subprocess terminates,
         *     subprocess may block at writing to stdout or stderr. That means you need to
         *     simultaneously deal with the io and query the subprocess termination.
         */
        while (!poll())
        {
            communicate();
            std::this_thread::sleep_for(interval);
        }
        communicate();
    }

    void wait();

    void communicate(const char *input, int len);

    void communicate();

    void communicate(const std::string &input);

    void terminate() const;

    void kill() const;

    void send_signal(int sig) const;

    int returncode() const noexcept;

    const char *stdout() const noexcept;

    const char *stderr() const noexcept;

    pid_t pid() const noexcept;

private:
    std::string cmd_;

    char *const *envp_;

    std::unique_ptr<nstream> stdin_;

    std::unique_ptr<nstream> stdout_;

    std::unique_ptr<nstream> stderr_;

    pid_t pid_;

    int returncode_;
};

}   // namespace cppev

#endif  // subprocess.h
