#include <memory>
#include "LoaderTestImpl.h"

extern "C"
{

cppev::LoaderTestBase *LoaderTestBaseConstructorImpl()
{
    return new cppev::LoaderTestImpl();
}

void LoaderTestBaseDestructorImpl(cppev::LoaderTestBase *ptr)
{
    delete ptr;
}

std::shared_ptr<cppev::LoaderTestBase>
LoaderTestBaseSharedPtrConstructorImpl()
{
    return std::dynamic_pointer_cast<cppev::LoaderTestBase>(
        std::make_shared<cppev::LoaderTestImpl>());
}

}
