#include <thread>
#include <gtest/gtest.h>
#include "cppev/subprocess.h"

namespace cppev
{

class TestSubprocess
: public testing::Test
{
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(TestSubprocess, test_exec_cmd)
{
    std::tuple<int, std::string, std::string> rets;

    char *envp[] =
    {
        (char *)"test=TEST",
        nullptr
    };
    rets = subprocess::exec_cmd("/usr/bin/printenv test", envp);
    EXPECT_EQ(std::get<0>(rets), 0);
    EXPECT_STREQ(std::get<1>(rets).c_str(), "TEST\n");
    EXPECT_STREQ(std::get<2>(rets).c_str(), "");

    rets = subprocess::exec_cmd("/bin/cat /cppev/test/not/exist");
    EXPECT_NE(std::get<0>(rets), 0);
    EXPECT_STREQ(std::get<1>(rets).c_str(), "");
    EXPECT_STRNE(std::get<2>(rets).c_str(), "");
}

TEST_F(TestSubprocess, test_subp_popen)
{
    subp_open subp("/bin/cat");
    subp.communicate("cppev");

    // waiting for subprocess to process the data
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // terminate subprocess
    subp.terminate();

    // fetch output
    subp.wait();

    EXPECT_EQ(subp.returncode(), SIGTERM);
    EXPECT_STREQ(subp.stdout(), "cppev");
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}