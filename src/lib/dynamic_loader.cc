#include "cppev/dynamic_loader.h"

namespace cppev
{

dynamic_loader::dynamic_loader(const std::string &filename, dyld_mode mode)
    : handle_(nullptr)
{
    auto all_mode = RTLD_GLOBAL;
    if (mode == dyld_mode::lazy)
    {
        all_mode |= RTLD_LAZY;
    }
    else
    {
        all_mode |= RTLD_NOW;
    }
    handle_ = dlopen(filename.c_str(), all_mode);
    if (handle_ == nullptr)
    {
        throw_runtime_error(std::string("dlopen error : ").append(dlerror()));
    }
}

dynamic_loader::~dynamic_loader() noexcept
{
    dlclose(handle_);
}

}  // namespace cppev
