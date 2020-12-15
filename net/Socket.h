//
// Created by lenovo on 2020/12/10.
//

#ifndef TINYMUDUO_SOCKET_H
#define TINYMUDUO_SOCKET_H

#include "noncopyable.h"


class InetAddress;

///
/// Wrapper of socket file descriptor.
///
/// It closes the sockfd when desctructs.
/// It's thread safe, all operations are delagated to OS.
/// Socket 是给用户的接口，调用Socketops 和 InetAddress完成bind、listen、accept、shutdown，地址二进制和点分式转换等操作


class Socket : noncopyable
{
public:
    explicit Socket(int sockfd)
            : sockfd_(sockfd)
    { }

    ~Socket();

    int fd() const { return sockfd_; }

    /// abort if address in use
    void bindAddress(const InetAddress& localaddr);
    /// abort if address in use
    void listen();

    /// On success, returns a non-negative integer that is
    /// a descriptor for the accepted socket, which has been
    /// set to non-blocking and close-on-exec. *peeraddr is assigned.
    /// On error, -1 is returned, and *peeraddr is untouched.
    int accept(InetAddress* peeraddr);

    ///
    /// Enable/disable SO_REUSEADDR
    ///
    void setReuseAddr(bool on);

    void shutdownWrite();

private:
    const int sockfd_;
};

#endif //TINYMUDUO_SOCKET_H
