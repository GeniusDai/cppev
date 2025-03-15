#ifndef _cppev_subprocess_h_6C0224787A17_
#define _cppev_subprocess_h_6C0224787A17_

#include <chrono>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "cppev/io.h"

namespace cppev
{

namespace subprocess
{

CPPEV_PUBLIC std::tuple<int, std::string, std::string> exec_cmd(
    const std::string &cmd, const std::vector<std::string> &env = {});

}  // namespace subprocess

class CPPEV_PUBLIC subp_open final
{
public:
    explicit subp_open(const std::string &cmd,
                       const std::vector<std::string> &env);

    subp_open(const subp_open &) = delete;
    subp_open &operator=(const subp_open &) = delete;

    subp_open(subp_open &&other);
    subp_open &operator=(subp_open &&other);

    ~subp_open();

    bool poll();

    template <typename Rep, typename Period>
    void wait(const std::chrono::duration<Rep, Period> &interval)
    {
        /*
         * Q : Why polling is essential?
         * A : Buffer size of pipe is limited, so if you just wait for the
         * subprocess terminates, subprocess may block at writing to stdout or
         * stderr. That means you need to simultaneously deal with the io and
         * query the subprocess termination.
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

    void terminate();

    void kill();

    void send_signal(int sig);

    int returncode() const noexcept;

    const char *stdout() const noexcept;

    const char *stderr() const noexcept;

    pid_t pid() const noexcept;

private:
    std::string cmd_;

    std::vector<std::string> env_;

    std::unique_ptr<stream> stdin_;

    std::unique_ptr<stream> stdout_;

    std::unique_ptr<stream> stderr_;

    pid_t pid_;

    int returncode_;
};

}  // namespace cppev

#endif  // subprocess.h
