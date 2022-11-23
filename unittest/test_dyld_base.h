#include <string>
#include <memory>

namespace cppev
{

class LoaderTestBase
{
public:
    virtual ~LoaderTestBase() = default;
    virtual std::string add(int, int) const noexcept = 0;
    virtual std::string add(const std::string &, const std::string &) const noexcept = 0;

    virtual std::string type() const noexcept
    {
        return "base";
    }

    virtual std::string base() const noexcept
    {
        return "base";
    }

    void set_var(int var) noexcept
    {
        this->var = var;
    }

    int get_var() const noexcept
    {
        return var;
    }

private:
    int var;
};

typedef LoaderTestBase *LoaderTestBaseConstructorType();

typedef void LoaderTestBaseDestructorType(LoaderTestBase *);

// Not recommended due to warned by clang -Wreturn-type-c-linkage.
typedef std::shared_ptr<LoaderTestBase> LoaderTestBaseSharedPtrConstructorType();

}   // namespace cppev