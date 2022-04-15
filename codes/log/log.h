/*
 * @auther      : Parkertao
 * @Date        : 2022-3-17
 * @copyright Apache 2.0
*/

#ifndef LOG_H
#define LOG_H

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <cstdio>
#include <cstdarg>
#include <cassert>
#include <sys/stat.h>
#include "blockqueue.h"
#include "../buffer/buffer.h"


class Log {
public:
    void init(int level, const char* path = "./log", 
                const char* suffix = ".log", int max_queue_capacity = 1024);
    
    static Log* Instance();
    static void FlushLogThread();

    void WriteLog(int level, const char* format, ...);

    void Flush();

    int level();

    void SetLevel(int level);

    bool open() const { return opened_; }

private:
    Log();
    
    void AppendLogLevelTitle(int level);

    virtual ~Log();

    void AsyncWrite();


    // static const int log_path_length = 256;
    static const int log_name_length = 256;
    static const int max_lines = 50000;

    const char* path_;
    const char* suffix_;

    // int max_lines_;
    int line_count_;
    int today_;
    bool opened_;

    Buffer buffer_;
    int level_;
    bool is_async_;

    FILE* fp_;
    std::unique_ptr<BlockQueue<std::string>> deque_;
    std::unique_ptr<std::thread> write_thread_;
    std::mutex mtx_;
};

// 定义LOG等级
#define LOG_BASE(current_level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->open() && log->level() <= current_level) {\
            log->WriteLog(current_level, format, ##__VA_ARGS__); \
            log->Flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);


#endif

