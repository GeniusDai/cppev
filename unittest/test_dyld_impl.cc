#include "test_dyld_impl.h"

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

}
