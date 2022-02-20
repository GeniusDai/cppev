#include "cppev/subprocess.h"
#include "cppev/nio.h"
#include "cppev/common_utils.h"
#include <map>
#include <set>
#include <sys/wait.h>
#include <unistd.h>

namespace cppev
{

namespace subprocess
{

std::tuple<int, std::string, std::string> exec_cmd(const char *cmd, char *const *envp)
{
    std::vector<std::shared_ptr<nstream> > out = nio_factory::get_pipes();
    std::vector<std::shared_ptr<nstream> > err = nio_factory::get_pipes();

    pid_t pid = fork();

    if (pid == -1)
    {
        _exit(-1);
    }

    // child process
    if (pid == 0)
    {
        dup2(out[1]->fd(), STDOUT_FILENO);
        dup2(err[1]->fd(), STDERR_FILENO);

        std::vector<std::string> subs = utils::split(cmd, " ");
        std::vector<std::string> subs1 = utils::split(subs[0].c_str(), "/");
        char *argv[subs.size()+1];
        argv[subs.size()] = nullptr;
        argv[0] = const_cast<char *>(subs1[subs1.size()-1].c_str());
        for (int i = 1; i <= static_cast<int>(subs.size() - 1); ++i)
        {
            argv[i] = const_cast<char *>(subs[i].c_str());
        }
        execve(subs[0].c_str(), argv, envp);
        _exit(127);
    }

    // father process
    int status = -1;
    waitpid(pid, &status, 0);
    out[0]->read_all();
    err[0]->read_all();

    return std::make_tuple<>(status, out[0]->rbuf()->get(), err[0]->rbuf()->get());
}




}   // namespace subprocess

}   // namespace cppev
