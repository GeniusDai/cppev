#ifndef _dynamic_loader_h_6C0224787A17_
#define _dynamic_loader_h_6C0224787A17_

#include <string>
#include <dlfcn.h>
#include "cppev/utils.h"

namespace cppev
{

class dynamic_loader
{
public:
    explicit dynamic_loader(const std::string &filename)
    : handle_(nullptr)
    {
        handle_ = dlopen(filename.c_str(), RTLD_LAZY | RTLD_GLOBAL);
        if (handle_ == nullptr)
        {
            throw_runtime_error(std::string("dlopen error : ").append(dlerror()));
        }
    }

    dynamic_loader(const dynamic_loader&) = delete;
    dynamic_loader &operator=(const dynamic_loader&) = delete;
    dynamic_loader(dynamic_loader &&other) = delete;
    dynamic_loader &operator=(dynamic_loader &&other) = delete;

    ~dynamic_loader() noexcept
    {
        dlclose(handle_);
    }

    template <typename Function>
    Function *load(const std::string &func) const
    {
        void *ptr = dlsym(handle_, func.c_str());
        if (ptr == nullptr)
        {
            // errno is set in macOS, but not in linux
            throw_runtime_error(std::string("dlsym error : ").append(dlerror()));
        }
        return reinterpret_cast<Function *>(ptr);
    }

private:
    void *handle_;
};

} // namespace cppev

#endif  // dynamic_loader.h