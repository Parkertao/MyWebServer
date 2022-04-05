/*
 * @auther      : Parkertao
 * @Date        : 2022-3-20
 * @copyright Apache 2.0
*/

#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include <cassert>
#include "../log/log.h"

class SqlConnPool {
public:
    static SqlConnPool* Instance();

    MYSQL* GetSqlConn();
    void FreeSqlConn(MYSQL* conn);

    int free_count();

    void Init(const char* host, int port, const char* user, const char* password,
                const char* database_name, int conn_size = 10);

    void ClosePool();

private:
    SqlConnPool();
    ~SqlConnPool();

    int max_conn_;
    int used_conn_;
    int free_conn_;

    std::queue<MYSQL*> conn_queue_;
    std::mutex mtx_;
    sem_t sem_id_;

};


#endif  //SQLCONNPOOL_H