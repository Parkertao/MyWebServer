/*
 * @auther      : Parkertao
 * @Date        : 2022-3-24
 * @copyright Apache 2.0
*/

#include "httpconn.h"


bool HttpConn::ET_mode;
const char* HttpConn::kSrcDir;
std::atomic<int> HttpConn::user_count;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = { 0 };
    closed_ = true;
}

HttpConn::~HttpConn() {
    Close();
}
void HttpConn::Init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    user_count++;
    addr_ = addr;
    fd_ = fd;
    write_buffer_.RetrieveAll();
    read_buffer_.RetrieveAll();
    closed_ = false;
    LOG_INFO("Client[%d](%s:%d) in, user_count:%d", fd_, GetIP(), GetPort(), (int)user_count);
}

void HttpConn::Close() {
    response_.UnmapFile();
    if (closed_ == false)
    {
        closed_ = true;
        user_count--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, user_count:%d", fd_, GetIP(), GetPort(), (int)user_count);
    }
}

int HttpConn::GetFd() const {
    return fd_;
}

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

const char* HttpConn::GetIP() const {
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    return addr_.sin_port;
}

ssize_t HttpConn::read(int& save_errno) {
    ssize_t length = -1;
    do {
        length = read_buffer_.ReadFd(fd_, save_errno);
        if (length <= 0) break;
    } while (ET_mode);
    return length;
}

ssize_t HttpConn::write(int& save_errno) {
    ssize_t length = -1;
    do {
        length = writev(fd_, iovec_, iovec_count_);
        if (length <= 0)
        {
            save_errno = errno;
            break;
        }
        if (iovec_[0].iov_len + iovec_[1].iov_len == 0) break;
        else if (static_cast<size_t>(length) > iovec_[0].iov_len)
        {
            iovec_[1].iov_base = (uint8_t*) iovec_[1].iov_base + (length - iovec_[0].iov_len);
            iovec_[1].iov_len -= (length - iovec_[0].iov_len);
            if(iovec_[0].iov_len) {
                write_buffer_.RetrieveAll();
                iovec_[0].iov_len = 0;
            }
        }
        else {
            iovec_[0].iov_base = (uint8_t*)iovec_[0].iov_base + length; 
            iovec_[0].iov_len -= length; 
            write_buffer_.Retrieve(length);
        }
    } while (ET_mode || WriteableBytes() > 10240);
    return length;
}

bool HttpConn::Process() {
    request_.Init();
    if (read_buffer_.ReadableBytes() <= 0)
    {
        return false;
    }
    else if (request_.Parse(read_buffer_))
    {
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(kSrcDir, request_.path(), request_.alive(), 200);
    }
    else
    {
        response_.Init(kSrcDir, request_.path(), false, 400);
    }

    response_.MakeResponse(write_buffer_);
    // 响应头
    iovec_[0].iov_base = const_cast<char*>(write_buffer_.read_position());
    iovec_[0].iov_len = write_buffer_.ReadableBytes();
    iovec_count_ = 1;

    // 文件
    if (response_.FileLength() > 0 && response_.File())
    {
        iovec_[1].iov_base = response_.File();
        iovec_[1].iov_len = response_.FileLength();
        iovec_count_ = 2;
    }
    LOG_DEBUG("filesize: %d, %d to %d", response_.FileLength(), iovec_count_, WriteableBytes());
    return false;
}