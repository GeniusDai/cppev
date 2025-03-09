#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

#include "DynamicLoaderTestInterface.h"
#include "cppev/dynamic_loader.h"
#include "cppev/utils.h"

namespace cppev
{

std::string ld_full_name = std::string("libDynamicLoaderTestImplFunctions") +
#ifdef __linux__
                           ".so";
#else
                           ".dylib";
#endif  // __linux__

std::string ld_path;

static void test_class(DynamicLoaderTestInterface *base_cls)
{
    EXPECT_EQ(base_cls->add(66, 66), std::to_string(66 + 66));
    EXPECT_EQ(base_cls->add("66", "66"), "6666");
    EXPECT_EQ(base_cls->type(), "impl");
    EXPECT_EQ(base_cls->DynamicLoaderTestInterface::type(), "base");
    EXPECT_EQ(base_cls->base(), "base");
    EXPECT_EQ(base_cls->DynamicLoaderTestInterface::base(), "base");
    base_cls->set_var(66);
    EXPECT_EQ(base_cls->get_var(), 66);
}

class TestDynamicLoader : public testing::TestWithParam<std::tuple<std::string>>
{
};

TEST_P(TestDynamicLoader, test_base_impl_new_delete_loader)
{
    auto p = GetParam();

    dynamic_loader dyld(std::get<0>(p), dyld_mode::lazy);

    auto *constructor = dyld.load<DynamicLoaderTestInterfaceConstructorType>(
        "DynamicLoaderTestImplConstructor");
    auto *destructor = dyld.load<DynamicLoaderTestInterfaceDestructorType>(
        "DynamicLoaderTestImplDestructor");

    DynamicLoaderTestInterface *base_cls = constructor();

    test_class(base_cls);

    destructor(base_cls);
}

TEST_P(TestDynamicLoader, test_base_impl_shared_ptr_loader)
{
    auto p = GetParam();
    dynamic_loader dyld(std::get<0>(p), dyld_mode::now);

    auto *shared_ptr_constructor =
        dyld.load<DynamicLoaderTestInterfaceSharedPtrConstructorType>(
            "DynamicLoaderTestImplSharedPtrConstructor");

    std::shared_ptr<DynamicLoaderTestInterface> shared_base_cls =
        shared_ptr_constructor();

    test_class(shared_base_cls.get());
}

INSTANTIATE_TEST_SUITE_P(CppevTest, TestDynamicLoader,
#ifdef CPPEV_TEST_ENABLE_DLOPEN_ENV_SEARCH
                         testing::Combine(testing::Values(ld_path,
                                                          ld_full_name))
#else
                         testing::Combine(testing::Values(ld_path))
#endif
);

}  // namespace cppev

int main(int argc, char **argv)
{
    cppev::ld_path = std::string(std::filesystem::path(argv[0]).parent_path()) +
                     "/" + cppev::ld_full_name;
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
