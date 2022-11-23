#include "test_dyld_base.h"

namespace cppev
{

class LoaderTestImpl
: public LoaderTestBase
{
public:
    std::string add(int x, int y) const noexcept override
    {
        return std::to_string(x + y);
    }

    std::string add(const std::string &x, const std::string &y) const noexcept override
    {
        return x + y;
    }

    std::string type() const noexcept override
    {
        return "impl";
    }
};

}   // namespace cppev