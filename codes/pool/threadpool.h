/*
 * @auther      : Parkertao
 * @Date        : 2022-3-17
 * @copyright Apache 2.0
*/

#ifndef THREADPOLL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <cassert>

class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count = 8) : pool_(std::make_shared<Pool>()) {
        assert(thread_count > 0);
        for (size_t i = 0; i < thread_count; i++)
        {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> locker(pool -> mtx);
                while (true)
                {
                    if (!pool -> tasks.empty())
                    {
                        auto task = std::move(pool -> tasks.front());
                        pool -> tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    }
                    else if (pool -> closed) break;
                    else pool -> cond.wait(locker);
                }
            }).detach();
        }
    }

    // 默认构造函数
    ThreadPool() = default;
    // 移动构造函数
    ThreadPool(ThreadPool&&) = default;

    ~ThreadPool() {
        if (static_cast<bool>(pool_))
        {
            std::lock_guard<std::mutex> locker(pool_ -> mtx);
            pool_ -> closed = true;
        }
        pool_ -> cond.notify_all();
    }

    template <typename F>
    void AddTask(F&& task) {
        std::lock_guard<std::mutex> lokcer(pool_ -> mtx);
        pool_ -> tasks.emplace(std::forward<F>(task));
        pool_ -> cond.notify_one();
    }

private:
    struct Pool
    {
        std::mutex mtx;
        std::condition_variable cond;
        bool closed;
        // 创建任务队列
        std::queue<std::function<void()>> tasks;
    };
    // 定义Pool类型智能指针
    std::shared_ptr<Pool> pool_;
};


#endif  //THREADPOOL_H