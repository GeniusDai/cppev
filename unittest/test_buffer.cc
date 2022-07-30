#include <vector>
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
    buf.put_string(str);
    EXPECT_EQ(str.size(), buf.size());
    EXPECT_EQ(str[3], buf[3]);
    int offset = 3;
    buf.get_string(offset, false);
    EXPECT_EQ(str.size(), buf.size());
    buf.get_string(offset, true);
    EXPECT_EQ(str.size() - offset, buf.size());
    EXPECT_STREQ(str.substr(offset, str.size() - offset).c_str(), buf.buf());
}

TEST_F(TestBuffer, test_copy_move)
{
    std::string str = "cppev";

    std::vector<buffer> vec;
    vec.emplace_back(0);    // move constructor
    vec.back().put_string(str);
    vec.push_back(vec[0]);  // copy constructor

    for (auto &b : vec)
    {
        b = vec[0];
    }

    for (auto &b : vec)
    {
        EXPECT_EQ(b.get_string(-1, false), str);
        EXPECT_EQ(b.get_string(-1, true), str);
    }

    buffer b;
    b.put_string(str);
    buffer a = std::move(b);    // move assignment
    EXPECT_EQ(a.get_string(-1, false), str);
    EXPECT_EQ(b.buf(), nullptr);
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}