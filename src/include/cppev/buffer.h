#ifndef _buffer_h_6C0224787A17_
#define _buffer_h_6C0224787A17_

#include <utility>
#include <memory>
#include <cstring>
#include <cstdlib>
#include "cppev/utils.h"

namespace cppev
{

template <typename Char>
class basic_buffer final
{
    // Q: Why the two classes should be friend?
    // A: To save a memory copy.
    friend class nstream;
    friend class nsockudp;
public:
    basic_buffer() noexcept
    : basic_buffer(1)
    {
    }

    explicit basic_buffer(int cap) noexcept
    : cap_(cap), start_(0), offset_(0)
    {
        if (cap_ < 1)
        {
            throw_logic_error("buffer size shall not be less than 1!");
        }
        buffer_ = std::make_unique<Char[]>(cap_);
        if (cap_)
        {
            memset(buffer_.get(), 0, cap_);
        }
    }

    basic_buffer(const basic_buffer &other) noexcept
    {
        copy(other);
    }

    basic_buffer &operator=(const basic_buffer &other) noexcept
    {
        copy(other);
        return *this;
    }

    basic_buffer(basic_buffer &&other) noexcept = default;

    basic_buffer &operator=(basic_buffer &&other) = default;

    ~basic_buffer() = default;

    const Char &operator[](int i) const noexcept
    {
        return buffer_[start_ + i];
    }

    Char &operator[](int i) noexcept
    {
        return buffer_[start_ + i];
    }

    int size() const noexcept
    {
        return offset_ - start_;
    }

    int capacity() const noexcept
    {
        return cap_;
    }

    const Char *rawbuf() const noexcept
    {
        return buffer_.get() + start_;
    }

    Char *rawbuf() noexcept
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
        std::unique_ptr<Char[]> nbuffer = std::make_unique<Char[]>(cap_);
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

    // Produce Chars to buffer.
    // @param ptr : Pointer to Char array.
    // @param len : Char array length that copies to buffer.
    void produce(const Char *ptr, int len) noexcept
    {
        resize(offset_ + len);
        for (int i = 0; i < len; ++i)
        {
            buffer_[offset_++] = ptr[i];
        }
    }

    // Consume Chars from buffer
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

    // Produce string to buffer.
    // @param str : string to put.
    void put_string(const std::string &str) noexcept
    {
        produce(str.c_str(), str.size());
    }

    // Get string from buffer.
    // @param len: Char array length that consumes, -1 means all.
    // @param remove : whether consumes the Char array.
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
    std::unique_ptr<Char[]> buffer_;

    // Copy function for copy contructor and copy assignment
    void copy(const basic_buffer &other) noexcept
    {
        if (&other != this)
        {
            this->cap_ = other.cap_;
            this->start_ = other.start_;
            this->offset_ = other.offset_;
            this->buffer_ = std::make_unique<Char[]>(cap_);
            memcpy(this->buffer_.get(), other.buffer_.get(), cap_);
        }
    }
};

using buffer = basic_buffer<char>;

}   // namespace cppev

#endif  // buffer.h
