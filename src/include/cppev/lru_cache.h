#ifndef _lru_cache_h_6C0224787A17_
#define _lru_cache_h_6C0224787A17_

#include <list>
#include <unordered_map>
#include <tuple>
#include <climits>

namespace cppev
{

template<typename Key, typename Value>
class lru_cache final
{
public:
    explicit lru_cache(Value miss, int cap = INT_MAX)
    : miss_(miss), cap_(cap)
    {}

    Value get(Key key);

    void put(Key key, Value value);

private:
    std::list<std::pair<Key, Value> > cache_;

    std::unordered_map<Key, typename std::list<std::pair<Key, Value> >::iterator> hash_;

    Value miss_;

    int cap_;
};

template<typename Key, typename Value>
Value lru_cache<Key, Value>::get(Key key)
{
    if (hash_.count(key) == 0)
    {
        return miss_;
    }
    cache_.emplace_front(*hash_[key]);
    cache_.erase(hash_[key]);
    hash_[key] = cache_.begin();
    return cache_.front().second;
}

template<typename Key, typename Value>
void lru_cache<Key, Value>::put(Key key, Value value)
{
    if (hash_.count(key) != 0)
    {
        cache_.erase(hash_[key]);
        --cap_;
    }
    if (static_cast<int>(cache_.size()) == cap_)
    {
        hash_.erase(cache_.back().first);
        cache_.pop_back();
    }
    cache_.emplace_front(key, value);
    hash_[key] = cache_.begin();
}

}

#endif  // lru_cache.h
