/*
 * @auther      : Parkertao
 * @Date        : 2022-3-17
 * @copyright Apache 2.0
*/

#include "log.h"

Log::Log() {
    line_count_ = 0;
    is_async_ = false;
    write_thread_ = nullptr;
    deque_ = nullptr;
    today_ = 0;
    fp_ = nullptr;
}

Log::~Log() {
    if (write_thread_ && write_thread_ -> joinable())  //写线程可退出
    {
        while (!deque_ -> empty())
        {
            deque_ -> flush();  //
        }
        deque_ -> Close();
        write_thread_ -> join();
    }
    if (fp_) 
    {
        std::lock_guard<std::mutex> locker(mtx_);
        Flush();
        fclose(fp_);
    }
}

int Log::level() {
    std::lock_guard<std::mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    std::lock_guard<std::mutex> locker(mtx_);
    level_ = level;
}

void Log::AsyncWrite() {
    std::string str = "";
    // 从阻塞队列中取出一个日志字符串，写入文件
    while (deque_ -> pop(str))
    {
        std::lock_guard<std::mutex> locker(mtx_);
        fputs(str.c_str(), fp_);
    }
}

// C++11之后，使用局部变量懒汉不用加锁
Log* Log::Instance() {
    static Log instance;
    return &instance;
}

void Log::FlushLogThread() {
    Instance() -> AsyncWrite();
}
// 异步日志需要设置阻塞队列的长度，同步日志不需要
void Log::init (int level = 1, const char* path, const char* suffix, int max_queue_size) {
    opened_ = true;
    level_ = level;
    if (max_queue_size > 0)
    {
        // 异步模式
        is_async_ = true;
        if (!deque_) 
        {
            std::unique_ptr<BlockQueue<std::string>> new_deque(new BlockQueue<std::string>);
            deque_ = std::move(new_deque);

            std::unique_ptr<std::thread> new_thread(new std::thread(FlushLogThread));
            write_thread_ = std::move(new_thread);
        }
    }
    else
    {
        is_async_ = false;
    }

    line_count_ = 0;

    time_t timer = time(nullptr);
    struct tm *systime = localtime(&timer);
    struct tm t = *systime;

    path_ = path;
    suffix_ = suffix;
    char file_name[log_name_length] = {0};
    snprintf(file_name, log_name_length - 1, "%s/%04d_%02d_%02d%s", 
                path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    today_ = t.tm_mday;

    std::lock_guard<std::mutex> locker(mtx_);
    buffer_.RetrieveAll();
    if (fp_)
    {
        Flush();
        fclose(fp_);
    }

    fp_ = fopen(file_name, "a");
    // 如果创建失败
    if (fp_ == nullptr)
    {
        mkdir(path_, 0777);
        fp_ = fopen(file_name, "a");
    }
    assert(fp_);
}

void Log::WriteLog(int level, const char* format, ...) {
    struct timeval now = {0, 0};
    // 
    gettimeofday(&now, nullptr);
    time_t t_second = now.tv_sec;
    // 
    struct tm *systime = localtime(&t_second);
    struct tm t = *systime;
    

    // 日志日期不对或内容已满，新建一个日志文件
    if (today_ != t.tm_mday || (line_count_ && (line_count_ % max_lines == 0)))
    {
        std::unique_lock<std::mutex> locker(mtx_);
        locker.unlock();

        char new_file[log_name_length];
        char tail[36] = {0};
        // 
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year, t.tm_mon, t.tm_mday);

        if (today_ != t.tm_mday)
        {
            snprintf(new_file, log_name_length - 72, "%s/%s%s", path_, tail, suffix_);
            today_ = t.tm_mday;
            line_count_ = 0;
        }
        else
        {
            snprintf(new_file, log_name_length - 72, "%s/%s-%d%s", path_, tail, 
                        (line_count_ / max_lines), suffix_);
        }

        locker.lock();
        Flush();
        fclose(fp_);
        fp_ = fopen(new_file, "a");
        assert(fp_);
    }

    // 写操作
    std::unique_lock<std::mutex> locker(mtx_);
    line_count_++;
    int n = snprintf(const_cast<char*>(buffer_.write_position()), 
                        128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                        t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                        t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);

    buffer_.HasWritten(n);
    AppendLogLevelTitle(level);

    va_list valist;
    va_start(valist, format);
    int m = vsnprintf(const_cast<char*>(buffer_.write_position()), 
                        buffer_.WriteableBytes(), format, valist);
    va_end(valist);

    buffer_.HasWritten(m);
    buffer_.Append("\n\0", 2);

    // 如果采用异步模式，将buffer中的内容加入工作队列
    if (is_async_ && deque_ && !deque_ -> full())
    {
        deque_ -> push_back(buffer_.RetrieveAllToStr());
    }
    else
    {
        fputs(buffer_.read_position(), fp_);
    }
    buffer_.RetrieveAll();
}

void Log::AppendLogLevelTitle(int level) {
    switch (level)
    {
    case 0:
        buffer_.Append("[debug]: ", 9);
        break;
    case 1:
        buffer_.Append("[info]:  ", 9);
        break;
    case 2:
        buffer_.Append("[warn]:  ",9);
        break;
    case 3:
        buffer_.Append("[error]: ", 9);
        break;
    default:
        buffer_.Append("[info]:  ", 9);
        break;
    }
}

void Log::Flush() {
    if (is_async_)
    {
        deque_ -> flush();
    }
    // 强制刷新写入流缓冲区
    fflush(fp_);
}

