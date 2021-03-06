//
// Created by lenovo on 2020/12/8.
//

#ifndef TINYMUDUO_CHANNEL_H
#define TINYMUDUO_CHANNEL_H

//#include <boost/function.hpp>
//#include <boost/noncopyable.hpp>

#include <functional>
#include "noncopyable.h"

#include <Timestamp.h>
class EventLoop;

class Channel : public noncopyable
{
public:

    typedef std::function<void()> EventCallback;
    typedef std::function<void(muduo::Timestamp)> ReadEventCallback;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    ///活跃的channel处理读写事件统一调用接口
    void handleEvent(muduo::Timestamp receiveTime);
    void setReadCallback(const ReadEventCallback& cb) ///设置读事件回调函数，这里readcallback是需要一个参数的，但是cb如果是0个参数的函数也能编译运行
    { readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb) ///设置写事件回调函数
    { writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb) ///设置异常事件回调函数
    { errorCallback_ = cb; }
    void setCloseCallback(const EventCallback& cb)
    { closeCallback_ = cb; }

    ///过去channel状态接口
    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }


    ///开启、关闭channel读写事件接口
    void enableReading() { events_ |= kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    bool isWriting() const { return events_ & kWriteEvent; }

    /// for Poller 标记channel在poll的关注数组位置，或者epoll的状态
    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    ///返回所属的EventLoop
    EventLoop* ownerLoop() { return loop_; }

private:
    void update();

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int  fd_;
    int        events_;//关注的事件
    int        revents_;//活跃事件
    int        index_; // used by Poller.

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;

    bool eventHandling_;

};


#endif //TINYMUDUO_CHANNEL_H
