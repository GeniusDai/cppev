#include <gtest/gtest.h>
#include "cppev/lru_cache.h"

namespace cppev
{

class TestLRUCache
: public testing::Test
{
protected:
    void SetUp() override
    {}

    void TearDown() override
    {}
};

TEST_F(TestLRUCache, test_limit_cap)
{
    std::string null("cppev");
    lru_cache<int, std::string> lru(null, 2);
    lru.put(1, "1");
    ASSERT_EQ(null, lru.get(2));
    lru.put(2, "2");
    ASSERT_EQ("1", lru.get(1));
    lru.put(3, "3");
    ASSERT_EQ(null, lru.get(2));
    lru.put(1, "1-1");
    ASSERT_EQ("1-1", lru.get(1));
}

TEST_F(TestLRUCache, test_unlimit_cap)
{
    std::string null("cppev");
    lru_cache<int, std::string> lru(null);
    lru.put(1, "1");
    ASSERT_EQ(null, lru.get(2));
    lru.put(2, "2");
    ASSERT_EQ("1", lru.get(1));
    lru.put(3, "3");
    ASSERT_EQ("2", lru.get(2));
    lru.put(1, "1-1");
    ASSERT_EQ("1-1", lru.get(1));
}

}   // namespace cppev

int main(int argc, char **argv)
{
    testing::InitGoogleTest();
    return RUN_ALL_TESTS();
}