//
// Created by lenovo on 2020/12/8.
//

#include "Channel.h"
#include "EventLoop.h"
#include "Logging.h"

#include <sstream>

#include <poll.h>

using namespace muduo;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;  //读事件，POLLPRI紧急读事件tcp带外数据
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fdArg)
        : loop_(loop),
          fd_(fdArg),
          events_(0),
          revents_(0),
          index_(-1)
{
}

void Channel::update()
{
    loop_->updateChannel(this);
}

void Channel::handleEvent()
{
    if (revents_ & POLLNVAL) {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) readCallback_();
    }
    if (revents_ & POLLOUT) {
        if (writeCallback_) writeCallback_();
    }
}