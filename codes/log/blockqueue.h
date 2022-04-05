/*
 * @auther      : Parkertao
 * @Date        : 2022-3-17
 * @copyright Apache 2.0
*/

#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <deque>
#include <condition_variable>
// #include <sys/time.h>
#include <cassert>

template <typename T>
class BlockQueue {
public:
    explicit BlockQueue(size_t max_capacity = 1000);

    ~BlockQueue();

    void clear();

    bool empty();

    bool full();

    void Close();

    size_t size();

    size_t capacity();

    T front();

    T back();

    void push_back(const T& item);

    void push_front(const T& item);

    bool pop(T& item);

    bool pop(T& item, int timeout);

    void flush();

private:
    std::deque<T> deque_;

    size_t capacity_;

    std::mutex mtx_;

    bool closed_;

    std::condition_variable condition_consumer_;

    std::condition_variable condition_producer_;
};


template <typename T>
BlockQueue<T>::BlockQueue(size_t max_capacity) : capacity_(max_capacity) {
    assert(max_capacity > 0);
    closed_ = false;
}

template <typename T>
BlockQueue<T>::~BlockQueue() {
    Close();
}

template <typename T>
void BlockQueue<T>::Close() {
    std::lock_guard<std::mutex> lokcer(mtx_);
    deque_.clear();
    closed_ = true;
    condition_producer_.notify_all();
    condition_consumer_.notify_all();
}

template <typename T>
void BlockQueue<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx_);
    deque_.clear();
}

template <typename T>
void BlockQueue<T>::flush() {
    condition_consumer_.notify_one();
}

template <typename T>
T BlockQueue<T>::front() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.front();
}

template <typename T>
T BlockQueue<T>::back() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.back();
}

template <typename T>
size_t BlockQueue<T>::size() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.size();
}

template <typename T>
size_t BlockQueue<T>::capacity() {
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template <typename T>
void BlockQueue<T>::push_back(const T& item) {
    std::unique_lock<std::mutex> locker(mtx_);
    // 如果队列满，阻塞线程以等待队列不满
    while (deque_.size() >= capacity_)
    {
        condition_producer_.wait(locker);
    }
    deque_.push_back(item);
    condition_consumer_.notify_one();
}

template <class T>
void BlockQueue<T>::push_front(const T& item) {
    std::unique_lock<std::mutex> locker(mtx_);
    
    while(deque_.size() >= capacity_) {
        condition_producer_.wait(locker);
    }
    deque_.push_front(item);
    condition_consumer_.notify_one();
}

template <class T>
bool BlockQueue<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.empty();
}
// 判断队列满
template <class T>
bool BlockQueue<T>::full(){
    std::lock_guard<std::mutex> locker(mtx_);
    return deque_.size() >= capacity_;
}

template <class T>
bool BlockQueue<T>::pop(T& item) {
    std::unique_lock<std::mutex> locker(mtx_);
    // 队列空，则阻塞线程等待队列非空
    while(deque_.empty()){
        condition_consumer_.wait(locker);
        if(closed_){
            return false;
        }
    }
    item = deque_.front();
    deque_.pop_front();
    condition_producer_.notify_one();
    return true;
}

template <class T>
bool BlockQueue<T>::pop(T& item, int timeout) {
    // condition_variable条件变量的wait只支持unique_lock
    std::unique_lock<std::mutex> locker(mtx_);
    while(deque_.empty()){
        // 阻塞超时则返回失败
        if(condition_consumer_.wait_for(locker, std::chrono::seconds(timeout)) 
                == std::cv_status::timeout){
            return false;
        }
        if(closed_){
            return false;
        }
    }
    // 取出队列
    item = deque_.front();
    deque_.pop_front();
    condition_producer_.notify_one();
    return true;
}


#endif  // BLOCKQUEUE_H