#include "cppev/subprocess.h"
#include "cppev/nio.h"
#include "cppev/utils.h"
#include <map>
#include <set>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <thread>

#ifdef __APPLE__
extern char **environ;
#endif

namespace cppev
{

namespace subprocess
{

std::tuple<int, std::string, std::string> exec_cmd(const std::string &cmd, const std::vector<std::string> &env)
{
    subp_open subp(cmd, env);
    subp.wait();
    return std::make_tuple(subp.returncode(), subp.stdout(), subp.stderr());
}

}   // namespace subprocess

subp_open::subp_open(const std::string &cmd, const std::vector<std::string> &env)
: cmd_(cmd), env_(env)
{
    int fds[2];
    int zero, one, two;

    if (pipe(fds) < 0)
    {
        throw_system_error("pipe error");
    }
    zero = fds[0];
    stdin_ = std::make_unique<nstream>(fds[1]);

    if (pipe(fds) < 0)
    {
        throw_system_error("pipe error");
    }
    one = fds[1];
    stdout_ = std::make_unique<nstream>(fds[0]);

    if (pipe(fds) < 0)
    {
        throw_system_error("pipe error");
    }
    two = fds[1];
    stderr_ = std::make_unique<nstream>(fds[0]);

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

        std::vector<std::string> cmd_with_args = split(cmd_, " ");
        std::string cmd_with_path = cmd_with_args[0];
        cmd_with_args[0] = split(cmd_with_args[0], "/").back();

        char *argv[cmd_with_args.size()+1];
        memset(argv, 0, sizeof(argv));
        for (size_t i = 0; i < cmd_with_args.size(); ++i)
        {
            argv[i] = const_cast<char *>(cmd_with_args[i].c_str());
        }

        char *envp[env_.size()+1];
        memset(envp, 0, sizeof(envp));
        for (size_t i = 0; i < env_.size(); ++i)
        {
            envp[i] = const_cast<char *>(env_[i].c_str());
        }
        environ = envp;

        execvp(cmd_with_path.c_str(), argv);
        _exit(127);
    }

    pid_ = pid;
}

bool subp_open::poll()
{
    int ret = waitpid(pid_, &returncode_, WNOHANG);
    if (ret == -1)
    {
        _exit(-1);
    }
    return ret != 0;
}

void subp_open::wait()
{
    wait(std::chrono::milliseconds(50));
}

void subp_open::communicate(const char *input, int len)
{
    stdout_->read_all();
    stderr_->read_all();

    if (input != nullptr && len != 0)
    {
        stdin_->wbuffer().produce(input, len);
        stdin_->write_all();
    }
}

void subp_open::communicate()
{
    communicate(nullptr, 0);
}

void subp_open::communicate(const std::string &input)
{
    communicate(input.c_str(), input.size());
}

void subp_open::send_signal(int sig) const
{
    if (::kill(pid(), sig) < 0)
    {
        throw_system_error("kill error");
    }
}

void subp_open::terminate() const
{
    send_signal(SIGTERM);
}

void subp_open::kill() const
{
    send_signal(SIGKILL);
}

int subp_open::returncode() const noexcept
{
    return returncode_;
}

const char *subp_open::stdout() const noexcept
{
    return stdout_->rbuffer().rawbuf();
}

const char *subp_open::stderr() const noexcept
{
    return stderr_->rbuffer().rawbuf();
}

pid_t subp_open::pid() const noexcept
{
    return pid_;
}

}   // namespace cppev
