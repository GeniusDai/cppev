#include <gtest/gtest.h>

#include <vector>

#include "cppev/buffer.h"

namespace cppev
{

class TestBuffer : public testing::Test
{
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
    EXPECT_STREQ(str.substr(offset, str.size() - offset).c_str(), buf.data());
}

TEST_F(TestBuffer, test_resize_tiny_null)
{
    const char *str = "cppev\0cppev000";
    const int len = 11;

    buffer buf;
    buf.put_string(str, len);
    EXPECT_EQ(buf.size(), len);
    EXPECT_EQ(std::string(buf.data()), "cppev");
    EXPECT_EQ(buf.get_string(5), "cppev");
    EXPECT_EQ(buf.get_string(), std::string("\0cppev", len - 5));

    buf.put_string(str, len);
    buf.get_string(3);
    buf.tiny();
    EXPECT_EQ(std::string(buf.data()), "ev");
    buf.resize(16);
    EXPECT_EQ(std::string(buf.data()), "ev");
    buf.resize(1);
    EXPECT_EQ(std::string(buf.data()), "ev");
}

TEST_F(TestBuffer, test_copy_move)
{
    std::string str = "cppev";

    std::vector<buffer> vec;
    vec.emplace_back(1);  // move constructor
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
    buffer a = std::move(b);  // move assignment
    EXPECT_EQ(a.get_string(-1, false), str);
    EXPECT_EQ(b.data(), nullptr);
}

TEST_F(TestBuffer, test_compilation)
{
    basic_buffer<int> a;
    basic_buffer<double> b;

    struct ABC
    {
        int a;
        double b;
        long c;
    };
    basic_buffer<ABC> abc;
}

TEST_F(TestBuffer, test_ref)
{
    buffer b;

    EXPECT_EQ(b.get_offset(), 0);
    b.get_offset_ref() += 666;
    EXPECT_EQ(b.get_offset(), 666);

    b.get_start_ref() = 777;
    EXPECT_EQ(b.get_start(), 777);

    b.set_start(888);
    EXPECT_EQ(b.get_start(), 888);

    b.set_offset(999);
    EXPECT_EQ(b.get_offset(), 999);
}

TEST_F(TestBuffer, test_ptr_data)
{
    buffer b;
    b.put_string("cppev");
    EXPECT_EQ(b.ptr() + b.get_start(), b.data());

    b.clear();
    b.put_string("cppev", 1);
    EXPECT_EQ(b.size(), 1);
}

}  // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
