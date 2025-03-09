#include "DynamicLoaderTestImpl.h"

namespace cppev
{

std::string DynamicLoaderTestImpl::add(int x, int y) const noexcept
{
    return std::to_string(x + y);
}

std::string DynamicLoaderTestImpl::add(const std::string &x,
                                       const std::string &y) const noexcept
{
    return x + y;
}

std::string DynamicLoaderTestImpl::type() const noexcept
{
    return "impl";
}

}  // namespace cppev
