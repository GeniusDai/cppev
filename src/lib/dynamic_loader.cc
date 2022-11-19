#include "cppev/dynamic_loader.h"
#include "cppev/common_utils.h"

namespace cppev
{

dynamic_loader::dynamic_loader(const std::string &filename)
: handle_(nullptr)
{
    handle_ = dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (handle_ == nullptr)
    {
        throw_runtime_error(std::string("dlopen error : ").append(dlerror()));
    }
}

dynamic_loader::~dynamic_loader()
{
    dlclose(handle_);
}

} // namespace cppev
