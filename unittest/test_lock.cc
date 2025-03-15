#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>

#include "config.h"
#include "cppev/lock.h"

namespace cppev
{

}  // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
