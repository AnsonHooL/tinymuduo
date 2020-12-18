//
// Created by lenovo on 2020/12/18.
//

#ifndef TINYMUDUO_TCPCLIENT_H
#define TINYMUDUO_TCPCLIENT_H

#include "noncopyable.h"
#include "Mutex.h"
#include "TcpConnection.h"

#include <memory>

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient : noncopyable
{
public:
    TcpClient(EventLoop* loop,
              const InetAddress& serverAddr);

    ~TcpClient();  // force out-line dtor, for unique_ptr members.

    void connect();
    void disconnect();
    void stop();

    muduo::TcpConnectionPtr connection() const
    {
        muduo::MutexLockGuard lock(mutex_);
        return connection_;
    }

    bool retry() const;
    void enableRetry() { retry_ = true; }

    /// Set connection callback.
    /// Not thread safe.
    void setConnectionCallback(const muduo::ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    /// Set message callback.
    /// Not thread safe.
    void setMessageCallback(const muduo::MessageCallback& cb)
    { messageCallback_ = cb; }

    /// Set write complete callback.
    /// Not thread safe.
    void setWriteCompleteCallback(const muduo::WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

private:
    /// Not thread safe, but in loop
    void newConnection(int sockfd);
    /// Not thread safe, but in loop
    void removeConnection(const muduo::TcpConnectionPtr& conn);

    EventLoop* loop_;
    ConnectorPtr connector_; // avoid revealing Connector
    muduo::ConnectionCallback connectionCallback_;
    muduo::MessageCallback messageCallback_;
    muduo::WriteCompleteCallback writeCompleteCallback_;
    bool retry_;   // atmoic
    bool connect_; // atomic
    // always in loop thread
    int nextConnId_;
    mutable muduo::MutexLock mutex_;
    muduo::TcpConnectionPtr connection_; // @BuardedBy mutex_
};



































#endif //TINYMUDUO_TCPCLIENT_H
