/*
 * @auther      : Parkertao
 * @Date        : 2022-3-25
 * @copyright Apache 2.0
*/

#include "webserver.h"

WebServer::WebServer(
        int port, int trig_mode, int timeout_ms, bool open_linger,
        int sql_port, const char* sql_user, const char* sql_passwd,
        const char* databases_name, int conn_num, int thread_num,
        bool open_log, int log_level, int log_queue_size
        ) : port_(port), open_linger_(open_linger), timeout_ms_(timeout_ms), closed_(false),
            timer_(new Timer()), thread_pool_(new ThreadPool(thread_num)), epoller_(new Epoller()) {
    
    src_directory_ = getcwd(nullptr,256);
    assert(src_directory_);
    strncat(src_directory_, "/../resources/", 16);
    HttpConn::user_count = 0;
    HttpConn::kSrcDir = src_directory_;
    SqlConnPool::Instance() -> Init("localhost", sql_port, sql_user, sql_passwd, databases_name, conn_num);

    InitEventMode(trig_mode);
    if (!InitSocket()) closed_ = true;

    if (open_log)
    {
        Log::Instance() -> init(log_level, "./log", ".log", log_queue_size);
        if (closed_) 
        {
            LOG_ERROR("--------------Server Init Error!--------------");
        }
        else
        {
            LOG_INFO("--------------Server Init!------------------");
            LOG_INFO("Port:%d, OpenLinger: %s", port_, open_linger? "true":"false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listen_event_ & EPOLLET ? "ET": "LT"),
                            (conn_event_ & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", log_level);
            LOG_INFO("srcDir: %s", HttpConn::kSrcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", conn_num, thread_num);
        }
    }
}

WebServer::~WebServer() {
    close(listen_fd_);
    closed_ = true;
    free(src_directory_);
    SqlConnPool::Instance() -> ClosePool();
}


void WebServer::InitEventMode(int trig_mode) {
    listen_event_ = EPOLLRDHUP;
    conn_event_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trig_mode)
    {
    case 0:
        break;
    case 1:
        conn_event_ |= EPOLLET;
        break;
    case 2:
        listen_event_ |= EPOLLET;
        break;
    case 3:
        listen_event_ |= EPOLLET;
        conn_event_ |= EPOLLET;
        break;
    default:
        listen_event_ |= EPOLLET;
        conn_event_ |= EPOLLET;
        break;
    }
    HttpConn::ET_mode = (conn_event_ & EPOLLET);
}

void WebServer::Start() {
    int time_ms = -1;
    if (!closed_)
    {
        LOG_INFO("--------------Server Start!-------------");
    }
    while (!closed_) 
    {
        if (timeout_ms_ > 0)
        {
            time_ms = timer_ -> GetNextTick();
        }
        int event_count = epoller_ -> Wait(time_ms);
        for (int i = 0; i < event_count; i++)
        {
            int fd = epoller_ -> GetEventFd(i);
            uint32_t events = epoller_ -> GetEvents(i);
            if (fd == listen_fd_)
            {
                DealListen();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                assert(users_.count(fd));
                CloseConn(&users_[fd]);
            }
            else if (events & EPOLLIN)
            {
                assert(users_.count(fd));
                DealRead(&users_[fd]);
            }
            else if (events & EPOLLOUT)
            {
                assert(users_.count(fd));
                DealWrite(&users_[fd]);
            }
            else
            {
                LOG_ERROR("Unexcepted Event!");
            }
        }
    }
}

void WebServer::SendError(int fd, const char* info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0) LOG_WARN("send error to client[%d] error!", fd);
    close(fd);
}

void WebServer::CloseConn(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!",client -> GetFd());
    epoller_ -> DeleteFd(client -> GetFd());
    client -> Close();
}

void WebServer::AddClient(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    if (timeout_ms_ > 0)
    {
        timer_ -> push(fd, timeout_ms_, std::bind(&WebServer::CloseConn, this, &users_[fd]));
    }
    epoller_ -> AddFd(fd, EPOLLIN | conn_event_);
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

void WebServer::DealListen() {
    struct sockaddr_in addr;
    socklen_t length = sizeof(addr);
    do {
        int fd = accept(listen_fd_, (struct sockaddr *)&addr, &length);
        if (fd <= 0) 
        {
            LOG_DEBUG("Accept Error: %s", strerror(errno));
            return;
        }
        else if (HttpConn::user_count >= kMaxFd)
        {
            SendError(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient(fd, addr);
    } while (listen_event_ & EPOLLET);
}

void WebServer::DealRead(HttpConn* client) {
    assert(client);
    ExtentTime(client);
    thread_pool_ -> AddTask(std::bind(&WebServer::OnRead, this, client));
}

void WebServer::DealWrite(HttpConn* client) {
    assert(client);
    ExtentTime(client);
    thread_pool_ -> AddTask(std::bind(&WebServer::OnWrite, this, client));
}

void WebServer::ExtentTime(HttpConn* client) {
    assert(client);
    if (timeout_ms_ > 0) timer_ -> Adjust(client -> GetFd(), timeout_ms_);
}

void WebServer::OnRead(HttpConn* client) {
    assert(client);
    int ret = -1;
    int read_errno = 0;
    ret = client -> read(read_errno);
    if (ret <= 0 && read_errno != EAGAIN) 
    {
        CloseConn(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnWrite(HttpConn* client) {
    assert(client);
    int ret = -1, write_errno = 0;
    ret = client -> write(write_errno);
    if (client -> WriteableBytes() >= 0 && client -> alive())
    {
        OnProcess(client);
        return;
    }
    else if (ret < 0 && write_errno == EAGAIN)
    {
        epoller_ -> ModifyFd(client -> GetFd(), conn_event_ | EPOLLOUT);
        return;
    }
    CloseConn(client);
}

void WebServer::OnProcess(HttpConn* client) {
    assert(client);
    if (client -> Process())
        epoller_ -> ModifyFd(client -> GetFd(), conn_event_ | EPOLLOUT);
    else
        epoller_ -> ModifyFd(client -> GetFd(), conn_event_ | EPOLLIN);
}

bool WebServer::InitSocket() {
    int ret;
    struct sockaddr_in addr;
    if (port_ > 65535 || port_ < 1024)
    {
        LOG_ERROR("Port: %d Error!", port_);
        return false;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htonl(port_);
    struct linger optlinger = {0};
    // 优雅关闭连接：直到所有数据发送完毕或超时
    if (open_linger_) 
    {
        optlinger.l_onoff = 1;
        optlinger.l_linger = 1;
    }

    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0)
    {
        LOG_ERROR("Port: %d: Create socket error!", port_);
        return false;
    }

    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, &optlinger, sizeof(optlinger));
    if (ret < 0)
    {
        close(listen_fd_);
        LOG_ERROR("Port %d: Init linger error!", port_);
        return false;
    }

    int optval = 1;
    // 端口复用
    ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if (ret == -1)
    {
        LOG_ERROR("Set socket option error!");
        close(listen_fd_);
        return false;
    }

    ret = bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0)
    {
        LOG_ERROR("Port %d: bind error!", port_);
        close(listen_fd_);
        return false;
    }

    ret = listen(listen_fd_, 6);
    if (ret < 0)
    {
        LOG_ERROR("Add listen error!");
        close(listen_fd_);
        return false;
    }

    ret = epoller_ -> AddFd(listen_fd_, listen_event_ | EPOLLIN);
    if (ret == 0)
    {
        LOG_ERROR("Add listen error!");
        close(listen_fd_);
        return false;
    }
    SetFdNonblock(listen_fd_);
    LOG_INFO("Server port: %d", port_);
    return true;
}

int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}