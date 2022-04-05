/*
 * @auther      : Parkertao
 * @Date        : 2022-3-20
 * @copyright Apache 2.0
*/


#include "sqlconnpool.h"

SqlConnPool::SqlConnPool() : used_conn_(0), free_conn_(0) {}

SqlConnPool::~SqlConnPool() {
    ClosePool();
}

SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool conn_pool;
    return &conn_pool;
}

int SqlConnPool::free_count() {
    std::lock_guard<std::mutex> locker(mtx_);
    return free_conn_;
}

void SqlConnPool::Init(const char* host, int port, const char* user, const char* password,
                        const char* database_name, int conn_size) {
    assert(conn_size > 0);
    for (int i = 0; i < conn_size; i++)
    {
        MYSQL* sql = nullptr;
        sql = mysql_init(sql);
        if (!sql)
        {
            LOG_ERROR("MySql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host, user, password, database_name, port, nullptr, 0);
        if (!sql)
        {
            LOG_DEBUG("MySql connect error!");
            assert(sql);
        }
        conn_queue_.push(sql);
    }
    max_conn_ = conn_size;
    sem_init(&sem_id_, 0, max_conn_);
}

MYSQL* SqlConnPool::GetSqlConn() {
    MYSQL* sql = nullptr;
    if (conn_queue_.empty())
    {
        LOG_WARN("SqlConnPoll busy!")
        return nullptr;
    }
    // 信号量阻塞
    sem_wait(&sem_id_);
    std::lock_guard<std::mutex> locker(mtx_);
    sql = conn_queue_.front();
    conn_queue_.pop();
    return sql;
}

void SqlConnPool::FreeSqlConn(MYSQL* sql) {
    assert(sql);
    std::lock_guard<std::mutex> locker(mtx_);
    conn_queue_.push(sql);
    sem_post(&sem_id_);
}

void SqlConnPool::ClosePool() {
    std::lock_guard<std::mutex> locker(mtx_);
    while (!conn_queue_.empty())
    {
        auto item = conn_queue_.front();
        conn_queue_.pop();
        mysql_close(item);
    }
    mysql_library_end();
}