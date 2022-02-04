#include <gtest/gtest.h>
#include "cppev/nio.h"
#include <fcntl.h>

namespace cppev
{

const char *file = "./cppev_test_file";

const char *fifo = "./cppev_test_fifo";

const char *str = "Cppev is a C++ event driven library";

class TestNio
: public testing::Test
{
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}
};

TEST_F(TestNio, test_diskfile)
{
    int fd;

    fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    std::shared_ptr<nstream> iofw(new nstream(fd));
    fd = open(file, O_RDONLY);
    std::shared_ptr<nstream> iofr(new nstream(fd));

    iofw->wbuf()->put(str);
    iofw->write_all();
    iofw->close();
    iofr->read_all();
    EXPECT_TRUE(0 == strcmp(iofr->rbuf()->buf(), str));

    unlink(file);
}

TEST_F(TestNio, test_pipe)
{
    auto pipes = nio_factory::get_pipes();
    auto iopr = pipes[0];
    auto iopw = pipes[1];

    iopw->wbuf()->put(str);
    iopw->write_all();
    iopr->read_all();
    EXPECT_TRUE(0 == strcmp(str, iopr->rbuf()->buf()));
}

TEST_F(TestNio, test_fifo)
{
    auto fifos = nio_factory::get_fifos(fifo);
    auto iofr = fifos[0];
    auto iofw = fifos[1];
    iofw->wbuf()->put(str);
    iofw->write_all();
    iofr->read_all();
    EXPECT_TRUE(0 == strcmp(iofr->rbuf()->buf(), str));

    unlink(fifo);

}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}