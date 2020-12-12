//
// Created by lenovo on 2020/12/11.
//

#include "Acceptor.h"

#include "Logging.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include <functional>

using namespace muduo;

///Acceptor属于唯一一个IO线程负责accept连接
///创建服务端监听socket，bind服务端端口
///设置reuseaddr，重启时，即使处于Timewait时候也可以立即绑定。  TODO：那怎么识别过期信息：序号？
///Timewait作用：安全的关闭1连接和使所有数据包从互联网消失
///注册监听套接字的channel的可读回调函数
Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
        : loop_(loop),
          acceptSocket_(sockets::createNonblockingOrDie()),
          acceptChannel_(loop, acceptSocket_.fd()),
          listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(
            std::bind(&Acceptor::handleRead, this));
}

///监听socket，设置channel可读，
///channel可读说明有客户端连接进来
void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    InetAddress peerAddr(0);
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >=0)
    {
        if(newConnectionCallback_){
            newConnectionCallback_(connfd, peerAddr);
        } else
        {
            sockets::close(connfd);
        }
    }
}