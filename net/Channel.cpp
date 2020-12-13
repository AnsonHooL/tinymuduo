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

///指定channel关注的文件描述符fd，和所属的EventLoop
Channel::Channel(EventLoop* loop, int fdArg)
        : loop_(loop),
          fd_(fdArg),
          events_(0),
          revents_(0),
          index_(-1)
{
}

///断言channel析构的时候一定没有在处理事件
///防止处理事件过程,tcpconn close了自己
Channel::~Channel() { assert(!eventHandling_); }

///更新channel关注的事件
void Channel::update()
{
    loop_->updateChannel(this);
}

///channel响应可读可写事件还有error，执行注册的回调函数
void Channel::handleEvent()
{
    eventHandling_ = true;
    if (revents_ & POLLNVAL) {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        LOG_WARN << "Channel::handle_event() POLLHUP";
        if (closeCallback_) closeCallback_();
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
    eventHandling_ = false;
}