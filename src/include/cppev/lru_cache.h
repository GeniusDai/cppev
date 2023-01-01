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
    explicit lru_cache(Value null, long cap = LONG_MAX) noexcept
    : null_(null), cap_(cap)
    {
    }

    lru_cache(const lru_cache &) = delete;
    lru_cache &operator=(const lru_cache &) = delete;
    lru_cache(lru_cache &&) = delete;
    lru_cache &operator=(lru_cache &&) = delete;

    ~lru_cache() = default;

    Value get(Key key) noexcept;

    void put(Key key, Value value) noexcept;

private:
    std::list<std::pair<Key, Value>> cache_;

    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> hash_;

    Value null_;

    long cap_;
};

template<typename Key, typename Value>
Value lru_cache<Key, Value>::get(Key key) noexcept
{
    if (hash_.count(key) == 0)
    {
        return null_;
    }
    cache_.emplace_front(*hash_[key]);
    cache_.erase(hash_[key]);
    hash_[key] = cache_.begin();
    return cache_.front().second;
}

template<typename Key, typename Value>
void lru_cache<Key, Value>::put(Key key, Value value) noexcept
{
    if (hash_.count(key) != 0)
    {
        cache_.erase(hash_[key]);
        --cap_;
    }
    if (cache_.size() == static_cast<size_t>(cap_))
    {
        hash_.erase(cache_.back().first);
        cache_.pop_back();
    }
    cache_.emplace_front(key, value);
    hash_[key] = cache_.begin();
}

}   // namespace cppev

#endif  // lru_cache.h
