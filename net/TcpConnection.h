//
// Created by lenovo on 2020/12/12.
//

#ifndef TINYMUDUO_TCPCONNECTION_H
#define TINYMUDUO_TCPCONNECTION_H

#include "Callbacks.h"
#include "InetAddress.h"

#include <boost/any.hpp>
#include <noncopyable.h>
#include <memory>

class Channel;
class EventLoop;
class Socket;

namespace muduo {
///
/// TCP connection, for both client and server usage.
///

///muduo唯一使用shared_ptr管理的class，为了完全控制Tcpconn的生命周期
///原因是防止串话，如Tcpconn A让其他线程处理，然后再返回信息给客户端
///这时A关闭了，B新建连接，重复使用了文件描述符，有可能导致串话，所以必须有智能指针管理

///这里enable_shared_from_this一定要public继承，默认是private继承，只继承功能，不继承接口，就有大问题！！！！
///private继承可以转化为组合实现
    class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection> {

    public:
        /// Constructs a TcpConnection with a connected sockfd
        ///
        /// User should not create this object.
        TcpConnection(EventLoop *loop,
                      const std::string &name,
                      int sockfd,
                      const InetAddress &localAddr,
                      const InetAddress &peerAddr);

        ~TcpConnection();

        EventLoop *getLoop() const { return loop_; }

        const std::string &name() const { return name_; }

        const InetAddress &localAddress() { return localAddr_; }

        const InetAddress &peerAddress() { return peerAddr_; }

        bool connected() const { return state_ == kConnected; }

        void setConnectionCallback(const muduo::ConnectionCallback &cb) { connectionCallback_ = cb; }

        void setMessageCallback(const muduo::MessageCallback &cb) { messageCallback_ = cb; }

        /// Internal use only.

        /// called when TcpServer accepts a new connection
        void connectEstablished();   // should be called only once

    private:
        enum StateE {
            kConnecting, kConnected,
        };

        void setState(StateE s) { state_ = s; }

        void handleRead();

        EventLoop *loop_;
        std::string name_;
        StateE state_;  // FIXME: use atomic variable
        // we don't expose those classes to client.
        std::unique_ptr<Socket> socket_;
        std::unique_ptr<Channel> channel_;
        InetAddress localAddr_;
        InetAddress peerAddr_;
        muduo::ConnectionCallback connectionCallback_;
        muduo::MessageCallback messageCallback_;
        boost::any context_;
    };

};









#endif //TINYMUDUO_TCPCONNECTION_H
