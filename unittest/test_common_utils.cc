#include <gtest/gtest.h>
#include "cppev/common_utils.h"

namespace cppev
{

class TestCommonUtils
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

TEST_F(TestCommonUtils, test_split)
{
    std::vector<std::string> subs;

    subs = utils::split("hello, cppev", ",");
    EXPECT_EQ(subs.size(), 2);

    subs = utils::split("hello, cppev", ", ");
    EXPECT_EQ(subs.size(), 2);

    subs = utils::split(", hello, cppev, ", ", ");
    EXPECT_EQ(subs.size(), 4);
    EXPECT_STREQ(subs[0].c_str(), "");
    EXPECT_STREQ(subs[1].c_str(), "hello");
    EXPECT_STREQ(subs[2].c_str(), "cppev");
    EXPECT_STREQ(subs[3].c_str(), "");

    subs = utils::split(",hello,cppev,", ",");
    EXPECT_EQ(subs.size(), 4);
    EXPECT_STREQ(subs[0].c_str(), "");
    EXPECT_STREQ(subs[1].c_str(), "hello");
    EXPECT_STREQ(subs[2].c_str(), "cppev");
    EXPECT_STREQ(subs[3].c_str(), "");
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}