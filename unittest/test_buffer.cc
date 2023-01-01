#include <vector>
#include <gtest/gtest.h>
#include "cppev/buffer.h"

namespace cppev
{

class TestBuffer
: public testing::Test {
protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
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
    EXPECT_STREQ(str.substr(offset, str.size() - offset).c_str(), buf.rawbuf());
}

TEST_F(TestBuffer, test_resize_tiny_null)
{
    const char *str = "cppev\0cppev000";
    const int len = 11;

    buffer buf;
    buf.produce(str, len);
    EXPECT_EQ(buf.size(), len);
    EXPECT_EQ(std::string(buf.rawbuf()), "cppev");
    EXPECT_EQ(buf.get_string(5), "cppev");
    EXPECT_EQ(buf.get_string(), std::string("\0cppev", len - 5));

    buf.produce(str, len);
    buf.get_string(3);
    buf.tiny();
    EXPECT_EQ(std::string(buf.rawbuf()), "ev");
    buf.resize(16);
    EXPECT_EQ(std::string(buf.rawbuf()), "ev");
    buf.resize(1);
    EXPECT_EQ(std::string(buf.rawbuf()), "ev");
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
    EXPECT_EQ(b.rawbuf(), nullptr);
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}