#include <gtest/gtest.h>
#include "cppev/buffer.h"

namespace cppev
{

class TestBuffer
: public testing::Test {
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}
};

TEST_F(TestBuffer, test_put_get)
{
    buffer buf;
    std::string str = "Cppev is a C++ event driven library";
    buf.put(str);
    EXPECT_EQ(str.size(), buf.size());
    EXPECT_EQ(str[3], buf[3]);
    int offset = 3;
    buf.get(offset, false);
    EXPECT_EQ(str.size(), buf.size());
    buf.get(offset, true);
    EXPECT_EQ(str.size() - offset, buf.size());
    EXPECT_STREQ(str.substr(offset, str.size() - offset).c_str(), buf.buf());
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}