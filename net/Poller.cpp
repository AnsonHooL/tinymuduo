//
// Created by lenovo on 2020/12/8.
//

#include "Poller.h"

#include "Channel.h"
#include "Logging.h"

#include <assert.h>
#include <poll.h>

using namespace muduo;


Poller::Poller(EventLoop* loop)
        : ownerLoop_(loop)
{
}

Poller::~Poller()
{
}

Timestamp Poller::poll(int timeoutMs, ChannelList* activeChannels)
{
    // XXX pollfds_ shouldn't change
    int numEvents = ::poll(pollfds_.data(), pollfds_.size(), timeoutMs); //(pollfds数组，数组大小，超时时间) -> 活跃时间
    Timestamp now(Timestamp::now()); //返回处理时间
    if (numEvents > 0) {
        LOG_TRACE << numEvents << " events happended";
        fillActiveChannels(numEvents, activeChannels);   //填充活跃channel
    } else if (numEvents == 0) {
        LOG_TRACE << " nothing happended";
    } else {
        LOG_SYSERR << "Poller::poll()";
    }
    return now;
}

void Poller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (auto pfd = pollfds_.cbegin();
        pfd != pollfds_.cend() && numEvents > 0; ++pfd)
    {
        if(pfd->revents > 0)
        {
            --numEvents;
            ChannelMap::const_iterator ch = channels_.find(pfd->fd);
            assert(ch != channels_.end());
            Channel* channel = ch->second;
            assert(channel->fd() == pfd->fd);
            channel->set_revents(pfd->revents);
            // pfd->revents = 0;
            activeChannels->push_back(channel);
        }
    }
}



///新的channel添加到poll关注的struct数组中，保存fd-->channel的映射
///旧的channel则更新关注的事件
void Poller::updateChannel(Channel *channel)
{
    assertInLoopThread();
    LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
    if (channel->index() < 0)
    {
        // a new one, add to pollfds_
        assert(channels_.find(channel->fd()) == channels_.end());
        struct pollfd pfd;
        pfd.fd = channel->fd();
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        pollfds_.push_back(pfd);
        int idx = static_cast<int>(pollfds_.size())-1;
        channel->set_index(idx);
        channels_[pfd.fd] = channel;
    }
    else
    {
        // update existing one, 更新channel的关注事件，若不关注事件则设置其fd = -1
        assert(channels_.find(channel->fd()) != channels_.end());
        assert(channels_[channel->fd()] == channel);
        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        struct pollfd& pfd = pollfds_[idx];
        assert(pfd.fd == channel->fd() || pfd.fd == -1);
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
        if (channel->isNoneEvent()) {
            // ignore this pollfd
            pfd.fd = -1;
        }
    }
}