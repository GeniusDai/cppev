#include <gtest/gtest.h>
#include "cppev/dynamic_loader.h"
#include "test_dyld_base.h"

namespace cppev
{

TEST(TestDynamicLoader, test)
{
    std::string basename = "test_dyld_impl";
#ifdef __linux__
    std::string dyname = "./lib" + basename + ".so";
#else
    std::string dyname = "./lib" + basename + ".dylib";
#endif  // __linux__

    dynamic_loader dyld(dyname);
    auto *constructor = dyld.load<LoaderTestBaseConstructorType>("LoaderTestBaseConstructorImpl");
    auto *destructor = dyld.load<LoaderTestBaseDestructorType>("LoaderTestBaseDestructorImpl");

    LoaderTestBase *base_cls = constructor();

    EXPECT_EQ(base_cls->add(66, 66), std::to_string(66 + 66));
    EXPECT_EQ(base_cls->add("66", "66"), "6666");

    destructor(base_cls);
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
