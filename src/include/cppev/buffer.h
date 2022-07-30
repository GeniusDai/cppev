#ifndef _buffer_h_6C0224787A17_
#define _buffer_h_6C0224787A17_

#include <utility>
#include <memory>
#include <cstring>
#include "cppev/common_utils.h"

namespace cppev
{

class buffer final
{
    friend class nstream;
    friend class nsockudp;
public:
    buffer()
    : buffer(1)
    {}

    explicit buffer(int cap)
    : cap_(cap), start_(0), offset_(0)
    {
        buffer_ = std::unique_ptr<char[]>(new char[cap_]);
        if (cap_)
        {
            memset(buffer_.get(), 0, cap_);
        }
    }

    buffer(const buffer &other) noexcept
    {
        copy(other);
    }

    buffer &operator=(const buffer &other) noexcept
    {
        copy(other);
        return *this;
    }

    buffer(buffer &&other) noexcept
    {
        move(std::forward<buffer>(other));
    }

    buffer &operator=(buffer &&other) noexcept
    {
        move(std::forward<buffer>(other));
        return *this;
    }

    ~buffer() = default;

    char &operator[](int i)
    {
        return buffer_[start_ + i];
    }

    int size() const
    {
        return offset_ - start_;
    }

    int cap() const
    {
        return cap_;
    }

    char *buf()
    {
        return buffer_.get() + start_;
    }

    // Expand buffer
    void resize(int cap)
    {
        if (cap_ >= cap)
        {
            return;
        }
        if (0 == cap_)
        {
            cap_ = 1;
        }
        while(cap_ < cap)
        {
            cap_ *= 2;
        }
        std::unique_ptr<char[]> nbuffer = std::unique_ptr<char[]>(new char[cap_]);
        memset(nbuffer.get(), 0, cap_);
        for (int i = start_; i < offset_; ++i)
        {
            nbuffer[i] = buffer_[i];
        }
        buffer_ = std::move(nbuffer);
    }

    // Clear buffer
    void clear()
    {
        memset(buffer_.get(), 0, cap_);
        start_ = 0;
        offset_ = 0;
    }

    void put(const char *ptr, size_t len)
    {
        resize(offset_ + len);
        for (int i = 0; i < len; ++i)
        {
            buffer_[offset_++] = ptr[i];
        }
    }

    void put(const std::string &str)
    {
        put(str.c_str(), str.size());
    }

    std::string get(int len = -1, bool consume = true)
    {
        if (len == -1)
        {
            len = size();
        }
        std::string str(buffer_.get() + start_, len);
        if (consume)
        {
            start_ += len;
        }
        if (start_ == offset_)
        {
            clear();
        }
        return str;
    }

private:
    // Capacity, heap size
    int cap_;

    // Start of the buffer, this byte is included
    int start_;

    // End of the buffer, this byte is not included
    int offset_;

    // Heap buffer
    std::unique_ptr<char[]> buffer_;

    // Copy function for copy contructor and copy assignment
    void copy(const buffer &other) noexcept
    {
        if (&other != this)
        {
            this->cap_ = other.cap_;
            this->start_ = other.start_;
            this->offset_ = other.offset_;
            this->buffer_ = std::unique_ptr<char[]>(new char[cap_]);
            memcpy(this->buffer_.get(), other.buffer_.get(), cap_);
        }
    }

    // Swap function for move constructor and move assignment
    void move(buffer &&other) noexcept
    {
        this->cap_ = other.cap_;
        this->start_ = other.start_;
        this->offset_ = other.offset_;
        this->buffer_ = std::move(other.buffer_);
        other.cap_ = 0;
        other.start_ = 0;
        other.offset_ = 0;
    }
};

}   // namespace cppev

#endif  // buffer.h