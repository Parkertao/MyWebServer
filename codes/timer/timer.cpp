/*
 * @auther      : Parkertao
 * @Date        : 2022-3-24
 * @copyright Apache 2.0
*/

#include "timer.h"


void Timer::SwapNode(size_t index1, size_t index2)
{
    assert(index1 >= 0 && index1 < heap_.size());
    assert(index2 >= 0 && index2 < heap_.size());
    std::swap(heap_[index1], heap_[index2]);
    reference_[heap_[index1].id] = index1;
    reference_[heap_[index2].id] = index2;
}

// 向上调整节点
void Timer::SiftUp(size_t child)
{
    assert(child >= 0 && child < heap_.size());
    size_t parent = (child - 1) / 2;
    while ( parent >= 0)
    {
        if (heap_[parent] < heap_[child]) break;
        SwapNode(child, parent);
        child = parent; 
        parent = (child - 1) / 2;
    }
}

// 向下调整节点，成功返回true，失败返回false
bool Timer::SiftDown(size_t index, size_t floor) {
    assert(index >= 0 && index < heap_.size());
    assert(floor >= 0 && floor <= heap_.size());
    size_t parent = index;
    size_t child = parent * 2 + 1;
    while (child < floor)
    {
        if (child + 1 < floor && heap_[child + 1] < heap_[child]) child++;
        if (heap_[parent] < heap_[child]) break;
        SwapNode(parent, child);
        parent = child;
        child = index * 2 + 1;
    }
    return parent > index;
}

void Timer::push(int id, int timeout, const TimeoutCallBack& callback) {
    assert(id >= 0);
    size_t index;
    if (reference_.count(id))
    {
        // 已有节点，调整堆
        index = reference_[id];
        heap_[index].expires = Clock::now() + Ms(timeout);
        heap_[index].callback = callback;
        if (!SiftDown(index, heap_.size())) SiftUp(index);
    }
    else
    {
        // 新节点，队尾插入，再调整堆
        index = heap_.size();
        reference_[id] = index;
        heap_.push_back({id, Clock::now() + Ms(timeout), callback});
        SiftUp(index);
    }
}

void Timer::Work(int id) {
    if (heap_.empty() || !reference_.count(id)) return;

    size_t index = reference_[id];
    TimerNode node = heap_[index];
    node.callback();
    DeleteNode(index);
}

void Timer::DeleteNode(size_t index) {
    assert(!heap_.empty() && index >= 0 && index < heap_.size());
    // 将要删除的节点调整到队尾，再删除
    size_t floor = heap_.size() - 1;
    assert(index <= floor);
    if (index < floor)
    {
        SwapNode(index, floor);
        if (!SiftDown(index, floor)) SiftUp(index);
    }
    // 删除队尾节点
    reference_.erase(heap_.back().id);
    heap_.pop_back();
}

void Timer::Adjust(int id, int timeout) {
    assert(!heap_.empty() && reference_.count(id));
    size_t index = reference_[id];
    heap_[index].expires = Clock::now() + Ms(timeout);
    assert(SiftDown(index, heap_.size()));
}

void Timer::Tick() {
    // 清楚超时节点
    if (heap_.empty()) return;

    while (!heap_.empty())
    {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<Ms>(node.expires - Clock::now()).count() > 0)
            break;
        node.callback();
        pop();
    }
}

void Timer::pop() {
    assert(!heap_.empty());
    DeleteNode(0);
}

int Timer::GetNextTick() {
    Tick();
    size_t ret = -1;
    if (!heap_.empty()) 
    {
        ret = std::chrono::duration_cast<Ms>(heap_.front().expires - Clock::now()).count();
        if (ret < 0) ret = 0;
    }
    return ret;
}

void Timer::Clear() {
    reference_.clear();
    heap_.clear();
}