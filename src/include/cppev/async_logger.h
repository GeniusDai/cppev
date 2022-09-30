#ifndef _async_logger_h_6C0224787A17_
#define _async_logger_h_6C0224787A17_

#include <cstdio>

// Only use hashed async logger in linux with higher version
// glibc, lower version glibc or macOS got bug in read-write-lock
#if defined(__linux__) && defined(__GNUC_PREREQ)
# if __GNUC_PREREQ(2, 25)
# define __CPPEV_USE_HASHED_LOGGER__
# endif
#endif

#if defined(__CPPEV_USE_HASHED_LOGGER__)
# include "cppev/async_logger_hashed.h"
#else
# include "cppev/async_logger_buffered.h"
#endif

namespace cppev
{

namespace log
{
    extern async_logger info;
    extern async_logger error;
    extern async_logger endl;
}   // namespace log

}   // namespace cppev

#endif  // async_logger.h
