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

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}