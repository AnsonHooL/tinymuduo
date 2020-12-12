//
// Created by lenovo on 2020/12/12.
//

#include "TcpConnection.h"

#include "Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"

#include <functional>

#include <errno.h>
#include <stdio.h>

using namespace muduo;


TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &name,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
  : loop_(CHECK_NOTNULL(loop)),
    name_(name),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop,sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr)
{
    LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
              << " fd=" << sockfd;
    channel_->setReadCallback(
            std::bind(&TcpConnection::handleRead, this));
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
              << " fd=" << channel_->fd();
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->enableReading();

    if(connectionCallback_)
        connectionCallback_(shared_from_this());
}


///channel的可读回调函数，先把数据读进来
///再把数据传回注册的消息回调函数
void TcpConnection::handleRead()
{
    char buf[65536];
    ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
    if(messageCallback_)
        messageCallback_(shared_from_this(), buf, n);
    // FIXME: close connection if n == 0
}