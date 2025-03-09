#pragma once

#include <memory>
#include <string>

namespace cppev
{

class DynamicLoaderTestInterface
{
public:
    virtual ~DynamicLoaderTestInterface() = default;
    virtual std::string add(int, int) const noexcept = 0;
    virtual std::string add(const std::string &,
                            const std::string &) const noexcept = 0;

    virtual std::string type() const noexcept
    {
        return "base";
    }

    virtual std::string base() const noexcept
    {
        return "base";
    }

    void set_var(int var) noexcept
    {
        this->var = var;
    }

    int get_var() const noexcept
    {
        return var;
    }

private:
    int var;
};

using DynamicLoaderTestInterfaceConstructorType =
    DynamicLoaderTestInterface *();

using DynamicLoaderTestInterfaceDestructorType =
    void(DynamicLoaderTestInterface *);

// Not recommended due to warned by clang -Wreturn-type-c-linkage.
using DynamicLoaderTestInterfaceSharedPtrConstructorType =
    std::shared_ptr<DynamicLoaderTestInterface>();

}  // namespace cppev
