/*
 * @auther      : Parkertao
 * @Date        : 2022-3-25
 * @copyright Apache 2.0
*/

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <cassert>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../log/log.h"
#include "../timer/timer.h"
#include "../pool/threadpool.h"
#include "../pool/sqlconnpool.h"
#include "../http/httpconn.h"

class WebServer {
public:
    WebServer(
        int port, int trig_mode, int timeout_ms, bool open_linger,
        int sql_port, const char* sql_user, const char* sql_passwd,
        const char* databases_name, int conn_pool_num, int thread_num,
        bool open_log, int log_level, int log_queue_size
    );
    ~WebServer();
    void Start();

private:
    bool InitSocket();
    void InitEventMode(int );
    void AddClient(int , sockaddr_in );

    // 回头把指针改成引用看看能不能行
    void DealListen();
    void DealWrite(HttpConn* );
    void DealRead(HttpConn* );

    void SendError(int , const char* );
    void ExtentTime(HttpConn* );
    void CloseConn(HttpConn* );

    void OnRead(HttpConn* );
    void OnWrite(HttpConn* );
    void OnProcess(HttpConn* );

    static const int kMaxFd = 65536;
    static int SetFdNonblock(int );

    int port_;
    bool open_linger_;
    int timeout_ms_;
    bool closed_;
    int listen_fd_;
    char* src_directory_;

    uint32_t listen_event_;
    uint32_t conn_event_;

    std::unique_ptr<Timer> timer_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};



#endif  // WEBSERVER_H
