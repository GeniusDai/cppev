#include <memory>
#include <gtest/gtest.h>
#include "cppev/utils.h"
#include "cppev/dynamic_loader.h"
#include "LoaderTestBase.h"

namespace cppev
{

#ifdef __linux__
std::string ld_suffix = ".so";
#else
std::string ld_suffix = ".dylib";
#endif  // __linux__

std::string ld_full_name = "libLoaderTestFunctions" + ld_suffix;

static const std::string get_bin_directory_path(const std::string &exec_path)
{
    std::string path;

    for (int i = exec_path.size() - 1; i >= 0; --i)
    {
        if (exec_path[i] == '/')
        {
            path = exec_path.substr(0, i + 1);
            break;
        }
    }
    if (path.empty())
    {
        cppev::throw_runtime_error("find path error");
    }

    return path;
}

std::string ld_path = "";

static void test_class(LoaderTestBase *base_cls)
{
    EXPECT_EQ(base_cls->add(66, 66), std::to_string(66 + 66));
    EXPECT_EQ(base_cls->add("66", "66"), "6666");
    EXPECT_EQ(base_cls->type(), "impl");
    EXPECT_EQ(base_cls->LoaderTestBase::type(), "base");
    EXPECT_EQ(base_cls->base(), "base");
    EXPECT_EQ(base_cls->LoaderTestBase::base(), "base");
    base_cls->set_var(66);
    EXPECT_EQ(base_cls->get_var(), 66);
}

class TestDynamicLoader
: public testing::TestWithParam<std::tuple<std::string>>
{
};

TEST_P(TestDynamicLoader, test_base_impl_new_delete_loader)
{
    auto p = GetParam();

    dynamic_loader dyld(std::get<0>(p));

    auto *constructor = dyld.load<LoaderTestBaseConstructorType>("LoaderTestBaseConstructorImpl");
    auto *destructor = dyld.load<LoaderTestBaseDestructorType>("LoaderTestBaseDestructorImpl");

    LoaderTestBase *base_cls = constructor();

    test_class(base_cls);

    destructor(base_cls);
}

TEST_P(TestDynamicLoader, test_base_impl_shared_ptr_loader)
{
    auto p = GetParam();
    dynamic_loader dyld(std::get<0>(p));

    auto *shared_ptr_constructor = dyld.load<LoaderTestBaseSharedPtrConstructorType>("LoaderTestBaseSharedPtrConstructorImpl");

    std::shared_ptr<LoaderTestBase> shared_base_cls = shared_ptr_constructor();

    test_class(shared_base_cls.get());
}

INSTANTIATE_TEST_SUITE_P(CppevTest, TestDynamicLoader,
#ifdef CPPEV_TEST_ENABLE_DLOPEN_ENV_SEARCH
    testing::Combine(testing::Values(ld_path, ld_full_name))
#else
    testing::Combine(testing::Values(ld_path))
#endif
);

}   // namespace cppev

int main(int argc, char **argv)
{
    cppev::ld_path = cppev::get_bin_directory_path(argv[0]) + cppev::ld_full_name;

    std::cout << "dynamic library path : " << cppev::ld_path << std::endl;

    testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}
