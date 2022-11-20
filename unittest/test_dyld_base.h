#include <string>
#include <memory>

namespace cppev
{

class LoaderTestBase
{
public:
    virtual ~LoaderTestBase() = default;
    virtual std::string add(int, int) = 0;
    virtual std::string add(const std::string &, const std::string &) = 0;

    virtual std::string type()
    {
        return "base";
    }
};

typedef LoaderTestBase *LoaderTestBaseConstructorType();

typedef void LoaderTestBaseDestructorType(LoaderTestBase *);

// Not recommended due to warned by clang -Wreturn-type-c-linkage.
typedef std::shared_ptr<LoaderTestBase> LoaderTestBaseSharedPtrConstructorType();

}   // namespace cppev