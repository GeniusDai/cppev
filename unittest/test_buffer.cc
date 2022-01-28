#include <gtest/gtest.h>
#include "cppev/buffer.h"

namespace cppev {

class TestBuffer : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TestBuffer, test_put_get) {
    buffer buf;
    std::string str = "Cppev is a C++ event driven library";
    buf.put(str);
    ASSERT_EQ(str.size(), buf.size());
    ASSERT_EQ(str[3], buf[3]);
    int offset = 3;
    buf.get(offset, false);
    ASSERT_EQ(str.size(), buf.size());
    buf.get(offset, true);
    ASSERT_EQ(str.size() - offset, buf.size());
    ASSERT_FALSE(strcmp(str.substr(offset, str.size() - offset).c_str(), buf.buf()) );
}

}

int main(int argc, char **argv) {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}