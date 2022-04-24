/*
 * @auther      : Parkertao
 * @Date        : 2022-3-16
 * @copyright Apache 2.0
*/

#include "buffer.h"

Buffer::Buffer(int init_buffer_size) : buffer_(init_buffer_size), read_position_(0), write_position_(0) {}

size_t Buffer::ReadableBytes() const {
    return write_position_ - read_position_;
}

size_t Buffer::WriteableBytes() const {
    return buffer_.size() - write_position_;
}

size_t Buffer::PrependableBytes() const {
    return read_position_;
}

const char* Buffer::read_position() const {
    return buffer_begin() + read_position_;
}

const char* Buffer::write_position() const {
    return buffer_begin() + write_position_;
}

char* Buffer::buffer_begin() {
    return &*buffer_.begin();
}

const char* Buffer::buffer_begin() const {
    return &*buffer_.begin();
}
// char* Buffer::write_begin() {
//     return buffer_begin() + write_position_;
// }

void Buffer::Retrieve(size_t length) {
    assert(length <= ReadableBytes());
    read_position_ += length;
}

void Buffer::RetrieveUntil(const char* end) {
    assert(read_position() <= end);
    Retrieve(end - read_position());
}

void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    read_position_ = 0;
    write_position_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(read_position(), ReadableBytes());
    RetrieveAll();
    return str;
}

void Buffer::HasWritten(size_t length) {
    write_position_ += length;
}

void Buffer::MakeSpace(size_t length) {
    if (WriteableBytes() + PrependableBytes() >= length)
    {
        size_t readable = ReadableBytes();
        std::copy(buffer_begin() + read_position_, buffer_begin() + write_position_, buffer_begin());
        read_position_ = 0;
        write_position_ = read_position_ + readable;
        assert(readable == ReadableBytes());
    }
    else
    {
        buffer_.resize(write_position_ + length + 1);
    }
}

void Buffer::EnsureWriteable(size_t length) {
    if (WriteableBytes() < length) 
    {
        MakeSpace(length);
    }
    assert(WriteableBytes() >= length);
}

void Buffer:: Append(const char* str, size_t length) {
    assert(str);
    EnsureWriteable(length);
    std::copy(str, str + length, const_cast<char*>(write_position()));
    HasWritten(length);
}

void Buffer::Append(const void* data, size_t length) {
    assert(data);
    Append(static_cast<const char*>(data), length);
}

void Buffer::Append(const std::string& str) {
    Append(str.data(), str.length());
}

void Buffer::Append(const Buffer& buffer) {
    Append(buffer.read_position(), buffer.ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int& error_num) {
    char temp_buffer[65535];
    struct iovec iov[2];
    const size_t writeable_size = WriteableBytes();

    iov[0].iov_base = buffer_begin() + writeable_size;
    iov[0].iov_len = writeable_size;
    iov[1].iov_base = temp_buffer;
    iov[1].iov_len = sizeof(temp_buffer);

    const ssize_t length = readv(fd, iov, 2);
    if (length < 0) 
    {
        error_num = errno;
    }
    else if (static_cast<size_t>(length) <= writeable_size)
    {
        write_position_ += length;
    }
    else 
    {
        write_position_ = buffer_.size();
        Append(temp_buffer, length - writeable_size);
    }
    return length;
}

ssize_t Buffer::WriteFd(int fd, int& error_num) {
    size_t readable_size = ReadableBytes();
    ssize_t length = write(fd, read_position(), readable_size);
    if (length < 0) 
    {
        error_num = errno;
        return length;
    }
    read_position_ += length;
    return length;
}