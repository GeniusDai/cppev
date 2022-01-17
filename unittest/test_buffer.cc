#include <gtest/gtest.h>
#include "cppev/buffer.h"

using namespace std;
using namespace cppev;
using namespace testing;

class test_buffer : public Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(test_buffer, test_put_get) {
    buffer buf;
    string str = "cppev is an event driven lib";
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

int main(int argc, char **argv) {
    InitGoogleTest();
    return RUN_ALL_TESTS();
}