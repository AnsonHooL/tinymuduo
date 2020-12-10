//
// Created by lenovo on 2020/12/8.
//

#ifndef TINYMUDUO_POLLER_H
#define TINYMUDUO_POLLER_H

#include <map>
#include <vector>

#include "Timestamp.h"
#include "EventLoop.h"
#include "noncopyable.h"
struct pollfd;

class Channel;

///
/// IO Multiplexing with poll(2).
///
/// This class doesn't own the Channel objects.
/// 只是拿channel来用而已,Tcpconnect、EventLoop管理自己的channel


class Poller :public noncopyable
{

public:
    typedef std::vector<Channel*> ChannelList;

    Poller(EventLoop* loop);
    ~Poller();

    /// Polls the I/O events.
    /// Must be called in the loop thread.
    Timestamp poll(int timeoutMs, ChannelList* activeChannels);


    /// Changes the interested I/O events.
    /// Must be called in the loop thread.
    void updateChannel(Channel* channel);

    void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }

private:
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    typedef std::vector<struct pollfd> PollFdList;
    typedef std::map<int, Channel*> ChannelMap;

    EventLoop* ownerLoop_; //poll只属于一个eventloop
    PollFdList pollfds_;   //poll关注的fd list，对应增删channel操作，poll函数的参数
    ChannelMap channels_;  //<fd,channel>的map，保存着poll的channel，方便快速找到活跃事件fd对应的channel

};


#endif //TINYMUDUO_POLLER_H
