/*
 * @auther      : Parkertao
 * @Date        : 2022-3-24
 * @copyright Apache 2.0
*/

#include "epoller.h"

Epoller::Epoller(int max_events) : epoll_fd_(epoll_create(512)), events_(max_events) {
    assert(epoll_fd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller() {
    close(epoll_fd_);
}

bool Epoller::AddFd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event event = {0};
    event.data.fd = fd;
    event.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event);
}

bool Epoller::ModifyFd(int fd, uint32_t events) {
    if (fd < 0) return false;
    epoll_event event = {0};
    event.data.fd = fd;
    event.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
}

bool Epoller::DeleteFd(int fd) {
    if (fd < 0) return false;
    epoll_event event = {0};
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &event);
}

int Epoller::Wait(int timeout_ms) {
    return epoll_wait(epoll_fd_, &events_[0], static_cast<int>(events_.size()), timeout_ms);
}

int Epoller::GetEventFd(size_t index) const {
    assert(index < events_.size() && index >= 0);
    return events_[index].data.fd;
}

uint32_t Epoller::GetEvents(size_t index) const {
    assert(index < events_.size() && index >= 0);
    return events_[index].events;
}