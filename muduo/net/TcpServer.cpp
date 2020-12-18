//
// Created by lenovo on 2020/12/12.
//

#include "TcpServer.h"

#include "Logging.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "SocketsOps.h"

#include <functional>

#include <stdio.h>  // snprintf
using namespace std::placeholders;

///ip、port名字
///注册Acceptor的有新链接的回调函数：
///Acceptor用的是baseloop
///baseloop新建一个looppool
TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr)
        : loop_(CHECK_NOTNULL(loop)),
          name_(listenAddr.toHostPort()),
          acceptor_(new Acceptor(loop, listenAddr)),
          threadPool_(new EventLoopThreadPool(loop)),
          started_(false),
          nextConnId_(1)
{

    acceptor_->setNewConnectionCallback(
           std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

///Acceptor开始监听listen
void TcpServer::start()
{
    if (!started_)
    {
        started_ = true;
        threadPool_->start();
    }

    if (!acceptor_->listenning())
    {
        loop_->runInLoop(
                std::bind(&Acceptor::listen, acceptor_.get()));
    }
}


///这是acceptor新连接回调函数
///创建一个Tcpconnection，注册可读可写回调函数，close回调函数（用户不可知）
///Tcpconn设置建立连接
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    loop_->assertInLoopThread();
    char buf[32];
    snprintf(buf, sizeof buf, "#%d", nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_
             << "] - new connection [" << connName
             << "] from " << peerAddr.toHostPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    EventLoop* ioLoop = threadPool_->getNextLoop();
    TcpConnectionPtr conn(
            new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
            std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
    ioLoop->runInLoop(
            std::bind(&TcpConnection::connectEstablished, conn));
}



void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    // FIXME: unsafe
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();
    size_t n = connections_.erase(conn->name());
    assert(n == 1); (void)n;
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
}