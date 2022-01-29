#include <gtest/gtest.h>
#include "cppev/nio.h"
#include <fcntl.h>

namespace cppev {

const char *file = "./test_temp_file";
const char *str = "Cppev is a C++ event driven library";

class TestNio : public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(TestNio, test_diskfile) {
    int fd;

    fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    nstream *iofw = new nstream(fd);
    iofw->wbuf()->put(str);
    iofw->write_all();
    delete iofw;

    fd = open(file, O_RDONLY);
    nstream *iofr = new nstream(fd);
    iofr->read_all();
    EXPECT_TRUE(strcmp(iofr->rbuf()->buf(), str) == 0);
    delete iofr;

    unlink(file);
}

TEST_F(TestNio, test_pipe) {
    // int pfds[2];
    // pipe();
}

TEST_F(TestNio, test_fifo) {

}

}

int main(int argc, char **argv) {
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}