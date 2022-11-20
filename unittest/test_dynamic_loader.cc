#include <memory>
#include <gtest/gtest.h>
#include "cppev/dynamic_loader.h"
#include "test_dyld_base.h"

namespace cppev
{

std::string basename = "test_dyld_impl";
#ifdef __linux__
std::string dyname = "./lib" + basename + ".so";
#else
std::string dyname = "./lib" + basename + ".dylib";
#endif  // __linux__

// It's very hard to get the exact path of the .so due to compile toolchain !!

TEST(TestDynamicLoader, test_base_impl_loader)
{
    dynamic_loader dyld(dyname);
    auto *constructor = dyld.load<LoaderTestBaseConstructorType>("LoaderTestBaseConstructorImpl");
    auto *destructor = dyld.load<LoaderTestBaseDestructorType>("LoaderTestBaseDestructorImpl");

    LoaderTestBase *base_cls = constructor();

    EXPECT_EQ(base_cls->add(66, 66), std::to_string(66 + 66));
    EXPECT_EQ(base_cls->add("66", "66"), "6666");
    EXPECT_EQ(base_cls->type(), "impl");

    destructor(base_cls);
}

TEST(TestDynamicLoader, test_base_impl_shared_ptr_loader)
{
    dynamic_loader dyld(dyname);
    auto *shared_ptr_constructor = dyld.load<LoaderTestBaseSharedPtrConstructorType>("LoaderTestBaseSharedPtrConstructorImpl");

    std::shared_ptr<LoaderTestBase> base_cls = shared_ptr_constructor();

    EXPECT_EQ(base_cls->add(66, 66), std::to_string(66 + 66));
    EXPECT_EQ(base_cls->add("66", "66"), "6666");
    EXPECT_EQ(base_cls->type(), "impl");
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
