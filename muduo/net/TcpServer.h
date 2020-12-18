//
// Created by lenovo on 2020/12/12.
//

#ifndef TINYMUDUO_TCPSERVER_H
#define TINYMUDUO_TCPSERVER_H

#include "Callbacks.h"
#include "TcpConnection.h"
#include "InetAddress.h"

#include <map>
#include <noncopyable.h>
#include <memory>

///muduo一般都单向依赖，Tcpserver拥有Acceptor，Acceptor却不知道它的存在
///channel 和 poll就不是单向依赖的关系
class Acceptor;
class EventLoop;
class EventLoopThreadPool;




class TcpServer : noncopyable
{
public:
    TcpServer(EventLoop* loop, const InetAddress& listenAddr);
    //TODO:为啥一定要显式定义析构函数
    ///因为成员函数Acceptor是uniqueptr，其模板类型有二 1.class 2.class的删除器
    ///编译器uniqueptr析构的时候调用2，需要判断class impl，要实现了才能进行删除，所以内联了就不能删除
    ///所以要在.c文件outline定义析构函数就不会出错了
    ///注意，sharedptr的模板类型只要 1.class 用户可以指定不同的指针不同的析构方法
    ~TcpServer();  // force out-line dtor, for scoped_ptr members.

    /// Set the number of threads for handling input.
    ///
    /// Always accepts new connection in loop's thread.
    /// Must be called before @c start
    /// @param numThreads
    /// - 0 means all I/O in loop's thread, no thread will created.
    ///   this is the default value.
    /// - 1 means all I/O in another thread.
    /// - N means a thread pool with N threads, new connections
    ///   are assigned on a round-robin basis.
    void setThreadNum(int numThreads);

    /// Starts the server if it's not listenning.
    ///
    /// It's harmless to call it multiple times.
    /// Thread safe.
    void start();

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
    void newConnection(int sockfd, const InetAddress& peerAddr);
    /// Thread safe.
    void removeConnection(const muduo::TcpConnectionPtr& conn);
    /// Not thread safe, but in loop
    void removeConnectionInLoop(const muduo::TcpConnectionPtr& conn);

    typedef std::map<std::string, muduo::TcpConnectionPtr> ConnectionMap;

    EventLoop* loop_;  // the acceptor loop
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
    std::unique_ptr<EventLoopThreadPool> threadPool_;

    ///Acceptor有新连接时候，新建一个Tcpconnet，将用户注册的回调函数，传给Tcpconnet
    muduo::ConnectionCallback connectionCallback_;
    muduo::MessageCallback messageCallback_;
    muduo::WriteCompleteCallback writeCompleteCallback_;
    bool started_;
    int nextConnId_;  // always in loop thread
    ConnectionMap connections_;
};

























#endif //TINYMUDUO_TCPSERVER_H
