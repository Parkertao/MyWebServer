/*
 * @auther      : Parkertao
 * @Date        : 2022-3-16
 * @copyright Apache 2.0
*/

#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>  //perror
#include <iostream>
#include <unistd.h>
#include <sys/uio.h>
#include <vector>
#include <atomic>
#include <cassert>

class Buffer {
public:
    Buffer(int init_buffer_size = 1024);
    ~Buffer() = default;

    // 返回可读、可写、前置空间
    size_t WriteableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;

    
    void EnsureWriteable(size_t length);
    void HasWritten(size_t length);

    // 恢复
    void Retrieve(size_t length);
    void RetrieveUntil(const char* end);
    
    // 格式化缓冲区
    void RetrieveAll();
    // 将缓冲区数据保存至字符串返回并格式化缓冲区
    std::string RetrieveAllToStr();

    // 返回可写起始位置
    const char* write_position() const;
    // char* write_begin();
    // 返回可读起始位置
    const char* read_position() const;

    // 定义一组重载函数实现向缓冲区内填入数据
    void Append(const std::string& str);
    void Append(const char* str, size_t length);
    void Append(const void* data, size_t length);
    void Append(const Buffer& buffer);

    // 向文件描述符关联的IO或文件读写数据
    ssize_t ReadFd(int fd, int& error_num);
    ssize_t WriteFd(int fd, int& error_num);

private:
    // 获得缓冲区起始位置指针
    char* buffer_begin();
    const char* buffer_begin() const;
    // 获得填入数据所需要的空间，如果可用空间够，则移动未读数据，否则扩展缓冲区大小
    void MakeSpace(size_t length);

    std::vector<char> buffer_;
    std::atomic<std::size_t> read_position_;
    std::atomic<std::size_t> write_position_;
};

#endif //BUFFER_H