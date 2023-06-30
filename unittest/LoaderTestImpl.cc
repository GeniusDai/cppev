#include "LoaderTestImpl.h"

namespace cppev
{

std::string LoaderTestImpl::add(int x, int y) const noexcept
{
    return std::to_string(x + y);
}

std::string LoaderTestImpl::add(const std::string &x, const std::string &y) const noexcept
{
    return x + y;
}

std::string LoaderTestImpl::type() const noexcept
{
    return "impl";
}

}   // namespace cppev
