/*
 * @auther      : Parkertao
 * @Date        : 2022-3-24
 * @copyright Apache 2.0
*/

#ifndef TIMER_H
#define TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <cassert>
#include <chrono>

#include "../log/log.h"

using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using Ms = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack callback;
    bool operator <(const TimerNode& t)
    {
        return expires < t.expires;
    }
};

class Timer {
public:
    Timer() { heap_.reserve(64); }

    ~Timer() { Clear(); }

    void Adjust(int id, int new_expires);

    void push(int id, int timeout, const TimeoutCallBack& callback);

    void Work(int id);

    void Clear();
    
    void Tick();

    void pop();

    int GetNextTick();

private:
    void DeleteNode(size_t );

    void SiftUp(size_t );

    bool SiftDown(size_t , size_t );

    void SwapNode(size_t , size_t );

    std::vector<TimerNode> heap_;

    std::unordered_map<int, size_t> reference_;
};

#endif // TIMER_H