//
// Created by lenovo on 2020/12/12.
//

#include "TcpConnection.h"

#include "Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"

#include <functional>

#include <errno.h>
#include <stdio.h>

using namespace muduo;

///Tcpconn对应一个channel，设置其可读事件、可写事件
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
    channel_->setWriteCallback(
            std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
            std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
            std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
              << " fd=" << channel_->fd();
}

///连接建立，设置channel状态为可读，调用连接回调函数
void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->enableReading();

    if(connectionCallback_)
        connectionCallback_(shared_from_this());
}


/// called when TcpServer has removed me from its map
/// 当tcpserver从自己的map中注销了tcpconn后，将此函数放入EventLoop执行
/// Tcpconn析构前的最后一个成员函数
/// 设置关闭连接状态，调用连接回调函数
/// tcpconn -> eventloop -> poll 注销channel   封装屏蔽了poll（loop_->removeChannel）
/// channel的析构是发生在tcp析构时候，也就是这个函数执行完
void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnected);
    setState(kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());

    loop_->removeChannel(channel_.get());
}


///channel的可读回调函数，先把数据读进来
///再把数据传回注册的消息回调函数
void TcpConnection::handleRead()
{
    char buf[65536];
    ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
    if (n > 0) {
        messageCallback_(shared_from_this(), buf, n);
    } else if (n == 0) {
        handleClose();
    } else {
        handleError();
    }
}

void TcpConnection::handleWrite()
{
}

///可用于被动或者主动close连接，disableall设置channel不再关注事件
///并执行Tcpserver设置的close回调函数（1.server中注销tcpconn 2.延后tcpconn生命周期，真正析构发生在EventLoop的oneloop最后function）
void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpConnection::handleClose state = " << state_;
    assert(state_ == kConnected);
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    channel_->disableAll();
    // must be the last line
    closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
