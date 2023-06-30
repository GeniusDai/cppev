#include "LoaderTestBase.h"

namespace cppev
{

class LoaderTestImpl
: public LoaderTestBase
{
public:
    std::string add(int x, int y) const noexcept override;

    std::string add(const std::string &x, const std::string &y) const noexcept override;

    std::string type() const noexcept override;
};

}   // namespace cppev