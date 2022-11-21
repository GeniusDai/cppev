#ifndef _dynamic_loader_h_6C0224787A17_
#define _dynamic_loader_h_6C0224787A17_

#include <string>
#include <dlfcn.h>
#include "cppev/common_utils.h"

namespace cppev
{

class dynamic_loader
{
public:
    explicit dynamic_loader(const std::string &filename);

    dynamic_loader(const dynamic_loader&) = delete;
    dynamic_loader &operator=(const dynamic_loader&) = delete;

    dynamic_loader(dynamic_loader &&other) noexcept
    {
        this->handle_ = other.handle_;
        other.handle_ = nullptr;
    }

    dynamic_loader &operator=(dynamic_loader &&other) noexcept
    {
        this->handle_ = other.handle_;
        other.handle_ = nullptr;
        return *this;
    }

    ~dynamic_loader() noexcept;

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