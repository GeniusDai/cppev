#ifndef _subprocess_h_6C0224787A17_
#define _subprocess_h_6C0224787A17_

#include <vector>
#include <string>
#include <tuple>
#include <memory>
#include "cppev/nio.h"

namespace cppev
{

namespace subprocess
{

std::tuple<int, std::string, std::string> exec_cmd(const char *cmd, char *const *envp = nullptr);

class popen
: public uncopyable
{
public:
    popen(const char *cmd, char *const *envp=nullptr);

    bool poll();

    void wait();

    void commumicate(const char *str=nullptr);

    int returncode() const
    {
        return returncode_;
    }

    const char *stdout() const
    {
        return stdout_->rbuf()->buf();
    }

    const char *stderr() const
    {
        return stderr_->rbuf()->buf();
    }

    pid_t pid() const
    {
        return pid_;
    }

private:
    const char *cmd_;

    char *const *envp_;

    std::shared_ptr<nstream> stdin_;

    std::shared_ptr<nstream> stdout_;

    std::shared_ptr<nstream> stderr_;

    pid_t pid_;

    int returncode_;
};


}   // namespace subprocess

}   // namespace cppev

#endif  // subprocess.h
