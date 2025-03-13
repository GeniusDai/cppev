#include <gtest/gtest.h>

#include <filesystem>
#include <thread>

#include "cppev/subprocess.h"

namespace cppev
{

std::string dirpath;

TEST(TestSubprocessExecCmd, test_exec_cmd)
{
    std::tuple<int, std::string, std::string> rets;

    rets = subprocess::exec_cmd("printenv test", {"test=TEST"});
    EXPECT_EQ(std::get<0>(rets), 0);
    EXPECT_STREQ(std::get<1>(rets).c_str(), "TEST\n");
    EXPECT_STREQ(std::get<2>(rets).c_str(), "");

    rets = subprocess::exec_cmd("/bin/cat /cppev/test/not/exist");
    EXPECT_NE(std::get<0>(rets), 0);
    EXPECT_STREQ(std::get<1>(rets).c_str(), "");
    EXPECT_STRNE(std::get<2>(rets).c_str(), "");

    rets = subprocess::exec_cmd("not_exist_cmd /cppev/test/not/exist");
    EXPECT_NE(std::get<0>(rets), 0);
    EXPECT_STREQ(std::get<1>(rets).c_str(), "");
    EXPECT_STREQ(std::get<2>(rets).c_str(), "");
}

TEST(TestSubprocessExecCmd, test_exec_cmd_python)
{
    std::tuple<int, std::string, std::string> rets;

    std::string pyfile = dirpath + "/hello.py";

    rets = subprocess::exec_cmd(pyfile);
    EXPECT_EQ(std::get<0>(rets), 0);
    EXPECT_STREQ(std::get<1>(rets).c_str(), "hello\n");
    EXPECT_STREQ(std::get<2>(rets).c_str(), "");

    rets = subprocess::exec_cmd(std::string("python3 ") + pyfile);
    EXPECT_EQ(std::get<0>(rets), 0);
    EXPECT_STREQ(std::get<1>(rets).c_str(), "hello\n");
    EXPECT_STREQ(std::get<2>(rets).c_str(), "");
}

class TestSubprocess
    : public testing::TestWithParam<std::tuple<std::string, int>>
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_P(TestSubprocess, test_subp_popen)
{
    auto param = GetParam();

    subp_open subp("cat", {});
    subp.communicate(std::get<0>(param));

    subp_open subp1(std::move(subp));
    subp = std::move(subp1);

    // wait for subprocess to process the data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // terminate subprocess
    subp.send_signal(std::get<1>(param));

    // fetch output
    subp.wait();

    EXPECT_EQ(subp.stdout(), std::get<0>(param));
    EXPECT_EQ(subp.stderr(), std::string(""));
    EXPECT_NE(subp.pid(), getpid());
}

INSTANTIATE_TEST_SUITE_P(
    CppevTest, TestSubprocess,
    testing::Combine(testing::Values("cppev", "event driven"),   // input
                     testing::Values(SIGTERM, SIGKILL, SIGABRT)  // signal
                     ));

}  // namespace cppev

int main(int argc, char **argv)
{
    cppev::dirpath = std::string(std::filesystem::path(argv[0]).parent_path());
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
