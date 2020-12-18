//
// Created by lenovo on 2020/12/11.
//

#ifndef TINYMUDUO_ACCEPTOR_H
#define TINYMUDUO_ACCEPTOR_H

#include <functional>
#include <noncopyable.h>

#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : noncopyable
{
public:
    typedef std::function<void (int sockfd,
                                  const InetAddress&)> NewConnectionCallback;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr);

    void setNewConnectionCallback(const NewConnectionCallback& cb)
    { newConnectionCallback_ = cb; }

    bool listenning() const { return listenning_; }
    void listen();

private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};














#endif //TINYMUDUO_ACCEPTOR_H
