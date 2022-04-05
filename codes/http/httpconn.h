/*
 * @auther      : Parkertao
 * @Date        : 2022-3-24
 * @copyright Apache 2.0
*/

#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <atomic>

#include "../log/log.h"
// #include "../pool/sqlconnRAII.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {
public:
    HttpConn();
    ~HttpConn();

    void Init(int socket_fd, const sockaddr_in& addr);

    ssize_t read(int& save_errno);
    ssize_t write(int& save_errno);

    void Close();

    int GetFd() const;

    int GetPort() const;

    const char* GetIP() const;

    sockaddr_in GetAddr() const;

    bool Process();

    int WriteableBytes() {
        return iovec_[0].iov_len + iovec_[1].iov_len;
    }

    bool alive() {
        return request_.alive();
    }

    static bool ET_mode;
    static const char* kSrcDir;
    static std::atomic<int> user_count;

private:
    int fd_;
    struct sockaddr_in addr_;

    bool closed_;
    int iovec_count_;
    struct iovec iovec_[2];

    Buffer read_buffer_;
    Buffer write_buffer_;

    HttpRequest request_;
    HttpResponse response_;
};


#endif  // HTTPCONN_H