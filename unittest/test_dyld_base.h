#include <string>

namespace cppev
{

class LoaderTestBase
{
public:
    virtual ~LoaderTestBase() = default;
    virtual std::string add(int, int) = 0;
    virtual std::string add(const std::string &, const std::string &) = 0;
};

typedef LoaderTestBase *LoaderTestBaseConstructorType();

typedef void LoaderTestBaseDestructorType(LoaderTestBase *);

}   // namespace cppev