#ifndef _buffer_h_6C0224787A17_
#define _buffer_h_6C0224787A17_

#include <memory>
#include <cstring>
#include "cppev/common_utils.h"

namespace cppev {

class buffer final : public uncopyable {
    friend class nstream;
    friend class nsockudp;
private:
    // Capacity, heap size
    int cap_;

    // Start of the buffer, this byte is included
    int start_;

    // End of the buffer, this byte is not included
    int offset_;

    // Heap buffer
    std::unique_ptr<char[]> buffer_;

public:
    buffer() : buffer(1) {}

    explicit buffer(int cap) : cap_(cap), start_(0), offset_(0) {
        buffer_ = std::unique_ptr<char[]>(new char[cap_]);
        if (cap_) { memset(buffer_.get(), 0, cap_); }
    }

    char &operator[](int i) { return buffer_[start_ + i]; }

    int size() const { return offset_ - start_; }

    int cap() const { return cap_; }

    char *buf() { return buffer_.get() + start_; }

    // Expand buffer
    void resize(int cap) {
        if (cap_ >= cap) { return; }
        if (0 == cap_) { cap_ = 1; }
        while(cap_ < cap) { cap_ *= 2; }

        std::unique_ptr<char[]> nbuffer = std::unique_ptr<char[]>(new char[cap_]);
        memset(nbuffer.get(), 0, cap_);
        for (int i = start_; i < offset_; ++i) { nbuffer[i] = buffer_[i]; }
        buffer_ = std::move(nbuffer);
    }

    // Clear buffer
    void clear() {
        memset(buffer_.get(), 0, cap_);
        start_ = 0;
        offset_ = 0;
    }

    void put(const std::string &str) { put(str.c_str()); }

    void put(const char *str) {
        int len = strlen(str);
        resize(offset_ + len);
        for (int i = 0; i < len; ++i)
        { buffer_[offset_++] = str[i]; }
    }

    std::string get(int len = -1, bool consume = true) {
        if (len == -1) { len = size(); }
        std::string str(buffer_.get() + start_, len);
        if (consume) { start_ += len; }
        if (start_ == offset_) { clear(); }
        return str;
    }
};

}

#endif