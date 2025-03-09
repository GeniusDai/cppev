#pragma once

#include "DynamicLoaderTestInterface.h"

namespace cppev
{

class DynamicLoaderTestImpl : public DynamicLoaderTestInterface
{
public:
    std::string add(int x, int y) const noexcept override;

    std::string add(const std::string &x,
                    const std::string &y) const noexcept override;

    std::string type() const noexcept override;
};

}  // namespace cppev
