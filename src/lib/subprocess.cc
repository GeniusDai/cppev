#include "cppev/subprocess.h"
#include "cppev/nio.h"
#include "cppev/common_utils.h"
#include <map>
#include <set>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <thread>

namespace cppev
{

namespace subprocess
{

std::tuple<int, std::string, std::string> exec_cmd(const char *cmd, char *const *envp)
{
    popen subp(cmd, envp);
    subp.wait();
    return std::make_tuple<>(subp.returncode(), subp.stdout(), subp.stderr());
}

std::tuple<int, std::string, std::string> exec_cmd(const std::string &cmd, char *const *envp)
{
    return exec_cmd(cmd.c_str(), envp);
}

}   // namespace subprocess

popen::popen(const std::string &cmd, char *const *envp)
: cmd_(cmd), envp_(envp)
{
    int fds[2];
    int zero, one, two;

    if (pipe(fds) < 0)
    {
        throw_system_error("pipe error");
    }
    zero = fds[0];
    stdin_ = std::shared_ptr<nstream>(new nstream(fds[1]));

    if (pipe(fds) < 0)
    {
        throw_system_error("pipe error");
    }
    one = fds[1];
    stdout_ = std::shared_ptr<nstream>(new nstream(fds[0]));;

    if (pipe(fds) < 0)
    {
        throw_system_error("pipe error");
    }
    two = fds[1];
    stderr_ = std::shared_ptr<nstream>(new nstream(fds[0]));

    pid_t pid = fork();

    if (pid == -1)
    {
        _exit(-1);
    }

    if (pid == 0)
    {
        dup2(zero, STDIN_FILENO);
        dup2(one, STDOUT_FILENO);
        dup2(two, STDERR_FILENO);
        std::vector<std::string> subs = utils::split(cmd_, " ");
        std::vector<std::string> subs1 = utils::split(subs[0].c_str(), "/");
        char *argv[subs.size()+1];
        argv[subs.size()] = nullptr;
        argv[0] = const_cast<char *>(subs1[subs1.size()-1].c_str());
        for (int i = 1; i <= static_cast<int>(subs.size() - 1); ++i)
        {
            argv[i] = const_cast<char *>(subs[i].c_str());
        }
        execve(subs[0].c_str(), argv, envp_);
        _exit(127);
    }

    pid_ = pid;
}

bool popen::poll()
{
    int ret = waitpid(pid_, &returncode_, WNOHANG);
    if (ret == -1)
    {
        _exit(-1);
    }
    return ret != 0;
}

void popen::communicate(const std::string &str)
{
    stdout_->read_all();
    stderr_->read_all();

    if (str.size())
    {
        stdin_->wbuf()->put_string(str);
        stdin_->write_all();
    }
}

void popen::wait()
{
    while (!poll())
    {
        communicate();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    communicate();
}

void popen::send_signal(int sig)
{
    if (::kill(pid(), sig) < 0)
    {
        throw_system_error("kill error");
    }
}

void popen::terminate()
{
    send_signal(SIGTERM);
}

void popen::kill()
{
    send_signal(SIGKILL);
}

}   // namespace cppev
