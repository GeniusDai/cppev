#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <random>
#include <unistd.h>
#include <fcntl.h>
#include "cppev/sysconfig.h"
#include "cppev/async_logger.h"
#include "cppev/subprocess.h"

namespace cppev
{

const char *str = "Cppev is a C++ event driven library";

const char *file = "./cppev_test_logger_output.txt";

class TestAsyncLogger
: public testing::Test
{
protected:
    void SetUp() override
    {
        sysconfig::buffer_outdate = 1;
    }

    void TearDown() override
    {
    }

    int thr_num = 50;

    int loop_num = 100;
};

TEST_F(TestAsyncLogger, test_output)
{
    std::cout << async_logger::impl() << std::endl;
    int fd = open(file, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd < 0)
    {
        throw_system_error("open error");
    }

    int save_stdout = dup(STDOUT_FILENO);
    int save_stderr = dup(STDERR_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    auto output = [this](int i) -> void
    {
        for (int j = 0; j < this->loop_num; ++j)
        {
            log::info << str << " round [" << i << "]" << log::endl;
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            log::error << str << " round [" << i << "]" << log::endl;
        }
        std::random_device rd;
        std::default_random_engine rde(rd());
        std::uniform_int_distribution<int> dist(0, 1500);
        int value = dist(rde);
        std::this_thread::sleep_for(std::chrono::milliseconds(value));
        float x1 = value;
        double x2 = value;
        int x3 = value;
        long x4 = value;
        for (int j = 0; j < this->loop_num; ++j)
        {
            log::info
                << "Slept for "
                << x1 << " "
                << x2 << " "
                << x3 << " "
                << x4 << " "
            << log::endl;
            log::error
                << "Slept for "
                << x1 << " "
                << x2 << " "
                << x3 << " "
                << x4 << " "
            << log::endl;
        }
    };
    std::vector<std::thread> thrs;
    for (int i = 0; i < thr_num; ++i)
    {
        thrs.emplace_back(output, i);
    }
    for (int i = 0; i < thr_num; ++i)
    {
        thrs[i].join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    close(fd);
    dup2(save_stdout, STDOUT_FILENO);
    dup2(save_stderr, STDERR_FILENO);

    std::string cmd = std::string("/usr/bin/wc -l ") + file;
    auto ret = subprocess::exec_cmd(cmd);

    EXPECT_EQ(std::get<0>(ret), 0);
    EXPECT_NE(std::string::npos, std::get<1>(ret).find(std::to_string(loop_num * thr_num * 2 * 2)));
    EXPECT_EQ(std::get<2>(ret), "");
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}