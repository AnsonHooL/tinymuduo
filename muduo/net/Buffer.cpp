//
// Created by lenovo on 2020/12/15.
//

#include "Buffer.h"



#include "Buffer.h"
#include "SocketsOps.h"
#include "Logging.h"

#include <errno.h>
#include <memory.h>
#include <sys/uio.h>

using namespace muduo;


///在栈上有一个receive蓄水池extrabuf
///那么一开始buffer就不需要太大，当有很多信息过来，才扩大空间
///这里用途了readv函数，很赞的思路
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = begin()+writerIndex_;
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    const ssize_t n = readv(fd, vec, 2);
    if (n < 0) {
        *savedErrno = errno;
    } else if (implicit_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    } else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}