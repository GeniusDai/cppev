#include <memory>

#include "DynamicLoaderTestImpl.h"

extern "C"
{
    cppev::DynamicLoaderTestInterface *DynamicLoaderTestImplConstructor()
    {
        return new cppev::DynamicLoaderTestImpl();
    }

    void DynamicLoaderTestImplDestructor(cppev::DynamicLoaderTestInterface *ptr)
    {
        delete ptr;
    }

    std::shared_ptr<cppev::DynamicLoaderTestInterface>
    DynamicLoaderTestImplSharedPtrConstructor()
    {
        return std::dynamic_pointer_cast<cppev::DynamicLoaderTestInterface>(
            std::make_shared<cppev::DynamicLoaderTestImpl>());
    }
}
