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
using namespace std;

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
            std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)); ///std::bind一定要给够参数给绑定的函数，不够就用_1,_2
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

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::shutdown()
{
    // FIXME: use compare and swap
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

/// 这里会判断是否还在写决定shutdown，因为可能有效的数据还没发送完，所以这时候
/// 要等待数据发送完毕，在handlewrite里再调用一次shutdown才真正关闭写端口
void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
    {
        // we are not writing
        socket_->shutdownWrite();
    }
}

void TcpConnection::send(const std::string& message)
{
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            loop_->runInLoop(
                    std::bind(&TcpConnection::sendInLoop, this, message));
        }
    }
}

///先尝试直接发送一次，如果一次没写完，则用outputBuffer存取剩下的数据，设置tcpconn的channel关注写事件
///注意发送前要判断buffer里面如果有东西，则append，不能直接发送，会破坏顺序（TCP保证顺序到达，UDP不保证所以乱写吗）
///再等待channel可写事件活跃，channel进行handlewrite，将数据发送完毕
void TcpConnection::sendInLoop(const std::string& message)
{
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    // if no thing in output queue, try writing directly
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), message.data(), message.size());
        if (nwrote >= 0) {
            if (implicit_cast<size_t>(nwrote) < message.size()) {
                LOG_TRACE << "I am going to write more data";
            } else if(writeCompleteCallback_){
                loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::sendInLoop";
            }
        }
    }
    //TODO：这里可以直接发送一波数据吗，加速？
    assert(nwrote >= 0);
    if (implicit_cast<size_t>(nwrote) < message.size()) {
        outputBuffer_.append(message.data()+nwrote, message.size()-nwrote);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
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
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();
    if(connectionCallback_)
        connectionCallback_(shared_from_this());

    loop_->removeChannel(channel_.get());
}


///channel的可读回调函数，先把数据读进来
///再把数据传回注册的消息回调函数
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        handleError();
    }
}

///水平触发模式，因此写完，要关闭写事件关注
///这里只写一次，因为写一次写不完说明数据量挺大，通常再写也是 error
///当数据发送完毕后，查看是否处于关闭连接状态，若是则再一次进行shutdown
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        ssize_t n = ::write(channel_->fd(),
                            outputBuffer_.peek(),
                            outputBuffer_.readableBytes());
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if(writeCompleteCallback_){
                    loop_->queueInLoop(
                            std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            } else {
                LOG_TRACE << "I am going to write more data";
            }
        } else {
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    } else {
        LOG_TRACE << "Connection is down, no more writing";
    }
}

///可用于被动或者主动close连接，disableall设置channel不再关注事件
///并执行Tcpserver设置的close回调函数（1.server中注销tcpconn 2.延后tcpconn生命周期，真正析构发生在EventLoop的oneloop最后function）
void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpConnection::handleClose state = " << state_;
    assert(state_ == kConnected || state_ == kDisconnecting);
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
