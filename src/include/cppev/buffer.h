#ifndef _buffer_h_6C0224787A17_
#define _buffer_h_6C0224787A17_

#include <utility>
#include <memory>
#include <cstring>
#include <cstdlib>
#include "cppev/common_utils.h"

namespace cppev
{

class buffer final
{
    friend class nstream;
    friend class nsockudp;
public:
    buffer() noexcept
    : buffer(1)
    {
    }

    explicit buffer(int cap) noexcept
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

    const char &operator[](int i) const noexcept
    {
        return buffer_[start_ + i];
    }

    char &operator[](int i) noexcept
    {
        return buffer_[start_ + i];
    }

    int size() const noexcept
    {
        return offset_ - start_;
    }

    int cap() const noexcept
    {
        return cap_;
    }

    const char *rawbuf() const noexcept
    {
        return buffer_.get() + start_;
    }

    char *rawbuf() noexcept
    {
        return buffer_.get() + start_;
    }

    // Expand buffer
    void resize(int cap) noexcept
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

    // Move unconsumed buffer to the start
    void tiny() noexcept
    {
        if (start_ == 0)
        {
            return;
        }
        int len = offset_ - start_;
        for (int i = 0; i < len; ++i)
        {
            buffer_[i] = buffer_[i + start_];
        }
        memset(buffer_.get() + len, 0, start_);
        start_ = 0;
        offset_ = len;
    }

    // Clear buffer
    void clear() noexcept
    {
        memset(buffer_.get(), 0, cap_);
        start_ = 0;
        offset_ = 0;
    }

    // Produce chars to buffer
    // @param ptr : Pointer to char array may contain '\0'
    // @param len : Char array length that copies to buffer
    void produce(const char *ptr, int len) noexcept
    {
        resize(offset_ + len);
        for (int i = 0; i < len; ++i)
        {
            buffer_[offset_++] = ptr[i];
        }
    }

    // Consume chars from buffer
    // @param len : Char array length that consumes, -1 means all.
    void consume(int len = -1) noexcept
    {
        if (len == -1)
        {
            len = size();
        }
        start_ += len;
        if (start_ == offset_)
        {
            clear();
        }
    }

    // Produce string to buffer
    // param str : string to put
    void put_string(const std::string &str) noexcept
    {
        produce(str.c_str(), str.size());
    }

    // Get string from buffer
    // param len: Char array length that consumes, -1 means all.
    // param remove : whether consumes the char array.
    std::string get_string(int len = -1, bool remove = true) noexcept
    {
        if (len == -1)
        {
            len = size();
        }
        std::string str(buffer_.get() + start_, len);
        if (remove)
        {
            consume(len);
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