/*
 * @auther      : Parkertao
 * @Date        : 2022-3-20
 * @copyright Apache 2.0
*/

#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H

#include "sqlconnpool.h"

// 资源再对象构造时初始化，在对象析构时释放
class SqlConnRAII {
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool* conn_pool)
    {
        assert(conn_pool);
        *sql = conn_pool -> GetSqlConn();
        sql_ = *sql;
        conn_pool_ = conn_pool;
    }

    ~SqlConnRAII() {
        if (sql_) conn_pool_ -> FreeSqlConn(sql_);
    }

private:
    MYSQL* sql_;
    SqlConnPool* conn_pool_;
};


#endif  //SQLCONNRAII_H