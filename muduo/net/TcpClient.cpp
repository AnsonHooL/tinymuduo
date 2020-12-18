//
// Created by lenovo on 2020/12/18.
//

#include "TcpClient.h"


#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include "Logging.h"

#include <functional>

using namespace muduo;
using namespace std::placeholders;
// TcpClient::TcpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

namespace detail
{

void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
    loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const ConnectorPtr& connector)
{
    //connector->
}

}


///初始化EventLoop，connector
TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr)
        : loop_(CHECK_NOTNULL(loop)),
          connector_(new Connector(loop, serverAddr)),
          retry_(false),
          connect_(true),
          nextConnId_(1)
{
    connector_->setNewConnectionCallback(
            std::bind(&TcpClient::newConnection, this, _1));
    // FIXME setConnectFailedCallback
    LOG_INFO << "TcpClient::TcpClient[" << this
             << "] - connector " << connector_.get();
}


//TODO：客户端突然杀掉进程的codepath是啥
/// 这里是Client先于Tcpconn断开连接！
/// 将Tcpconn放到Loop里面destroy，这有啥考究吗
/// 或许只是延后Tcpconn析构吗，是的吧，这可能就是防止Tcpclient析构的时候，Tcpconn调用closecallback，这就麻烦了，
/// 所以把tcpconn的closecallback改成只destroy自己
/// 反正Tcpconn析构的时候，会关闭socket，服务器能收到，而且client已经挂了就无所谓了
TcpClient::~TcpClient()
{
    LOG_INFO << "TcpClient::~TcpClient[" << this
             << "] - connector " << connector_.get();
    TcpConnectionPtr conn;
    {
        MutexLockGuard lock(mutex_);
        conn = connection_;
    }
    if (conn)
    {
        // FIXME: not 100% safe, if we are in different thread
        CloseCallback cb = std::bind(&::detail::removeConnection, loop_, _1);
        loop_->runInLoop(
                std::bind(&TcpConnection::setCloseCallback, conn, cb));
    }
    else
    {
        connector_->stop();
        // FIXME: HACK
        loop_->runAfter(1, std::bind(&::detail::removeConnector, connector_));
    }
}


void TcpClient::connect()
{
    // FIXME: check state
    LOG_INFO << "TcpClient::connect[" << this << "] - connecting to "
             << connector_->serverAddress().toHostPort();
    connect_ = true;
    connector_->start();
}

///这里可能多线程访问智能指针,必须加锁，进行保护
void TcpClient::disconnect()
{
    connect_ = false;

    {
        MutexLockGuard lock(mutex_);
        if (connection_)
        {
            connection_->shutdown();
        }
    }
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

///connector连接成功后，调用这个回调函数创建一个Tcpconn
///智能指针的访问必须加锁
void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toHostPort().c_str(), nextConnId_);
    ++nextConnId_;
    string connName = buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(new TcpConnection(loop_,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
            std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
    {
        MutexLockGuard lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}


///这是Tcpclient还存活的时候，Tcpconn断开连接的回调函数
///client析构后就不是这个了
///注意conn是智能指针
void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        MutexLockGuard lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_)
    {
        LOG_INFO << "TcpClient::connect[" << this << "] - Reconnecting to "
                 << connector_->serverAddress().toHostPort();
        connector_->restart();
    }
}
























