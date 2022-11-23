#include <memory>
#include <gtest/gtest.h>
#include "cppev/common_utils.h"
#include "cppev/dynamic_loader.h"
#include "test_dyld_base.h"

namespace cppev
{

std::string ld_path = "";

static const std::string get_ld_path(const std::string &exec_path)
{
    std::string path = "";
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

#ifdef __linux__
    std::string ld_suffix = ".so";
#else
    std::string ld_suffix = ".dylib";
#endif  // __linux__

    return path + "lib" + "test_dyld_impl" + ld_suffix;
}

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

TEST(TestDynamicLoader, test_base_impl_new_delete_loader)
{
    dynamic_loader dyld(ld_path);
    auto *constructor = dyld.load<LoaderTestBaseConstructorType>("LoaderTestBaseConstructorImpl");
    auto *destructor = dyld.load<LoaderTestBaseDestructorType>("LoaderTestBaseDestructorImpl");

    LoaderTestBase *base_cls = constructor();

    test_class(base_cls);

    destructor(base_cls);
}

TEST(TestDynamicLoader, test_base_impl_shared_ptr_loader)
{
    dynamic_loader dyld(ld_path);
    auto *shared_ptr_constructor = dyld.load<LoaderTestBaseSharedPtrConstructorType>("LoaderTestBaseSharedPtrConstructorImpl");

    std::shared_ptr<LoaderTestBase> shared_base_cls = shared_ptr_constructor();

    test_class(shared_base_cls.get());
}

}   // namespace cppev

int main(int argc, char **argv)
{
    cppev::ld_path = cppev::get_ld_path(argv[0]);
    std::cout << "dynamic library path : " << cppev::ld_path << std::endl;
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}
