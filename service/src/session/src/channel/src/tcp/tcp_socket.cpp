/*
 * Copyright (C) 2023-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * Description: tcp socket.
 * Author: renshuang
 * Create: 2023-09-25
 */

#include "tcp_socket.h"

#include "cast_engine_log.h"
#include "securec.h"

namespace OHOS {
namespace CastEngine {
namespace CastEngineService {
DEFINE_CAST_ENGINE_LABEL("Cast-TcpSocket");

TcpSocket::TcpSocket()
{
    socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ < RET_OK) {
        CLOGE("Create socket error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
    } else {
        CLOGI("Create socket success.");
    }
}

TcpSocket::~TcpSocket()
{
    Close();
}

int TcpSocket::Bind(const std::string &ip, int port)
{
    struct sockaddr_in sockaddr{};
    sockaddr.sin_family = AF_INET;
    if (ip.empty()) {
        sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
    }

    if (port == INVALID_PORT) {
        sockaddr.sin_port = htons(RANDOM_PORT);
    } else {
        sockaddr.sin_port = htons(port);
    }

    if (::bind(socket_, reinterpret_cast<struct sockaddr *>(&sockaddr), sizeof(sockaddr)) < RET_OK) {
        CLOGE("Socket bind error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return INVALID_PORT;
    }

    CLOGI("Socket bind success.");
    if (port == INVALID_PORT) {
        return GetBindPort();
    }

    return port;
}

int TcpSocket::GetBindPort()
{
    if (socket_ == INVALID_SOCKET) {
        CLOGE("Socket getBindPort error: socket_ is invalid");
        return INVALID_PORT;
    }
    struct sockaddr_in sockaddr{};
    socklen_t addrLen = sizeof(sockaddr);
    if (getsockname(socket_, reinterpret_cast<struct sockaddr *>(&sockaddr), &addrLen) < RET_OK) {
        CLOGE("Socket getBindPort error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return INVALID_PORT;
    }
    return ntohs(sockaddr.sin_port);
}

int TcpSocket::GetPeerPort(int fd)
{
    if (fd == INVALID_SOCKET) {
        CLOGE("Socket getPeerPort error: socket is invalid");
        return INVALID_PORT;
    }
    struct sockaddr_in sockaddr{};
    socklen_t addrLen = sizeof(sockaddr);
    if (getpeername(fd, reinterpret_cast<struct sockaddr *>(&sockaddr), &addrLen) < RET_OK) {
        CLOGE("Socket getPeerPort error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return INVALID_PORT;
    }
    return ntohs(sockaddr.sin_port);
}

int TcpSocket::GetSocketFd()
{
    return socket_;
}

bool TcpSocket::Listen(int backlog)
{
    if (::listen(socket_, backlog) < RET_OK) {
        CLOGE("Socket listen error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return false;
    }

    CLOGI("Socket listening...");
    return true;
}

bool TcpSocket::Connect(const std::string & ip, int port)
{
    struct sockaddr_in sockaddr{};
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(ip.c_str());
    sockaddr.sin_port = htons(port);
    if (::connect(socket_, reinterpret_cast<struct sockaddr *>(&sockaddr), sizeof(sockaddr)) < RET_OK) {
        CLOGE("Socket connect error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return false;
    }

    CLOGI("Socket connect success.");
    return true;
}

int TcpSocket::Accept()
{
    int connfd = ::accept(socket_, nullptr, nullptr);
    if (connfd < RET_OK) {
        CLOGE("Socket accept error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return INVALID_SOCKET;
    }

    CLOGI("Socket accept success.");
    return connfd;
}

int TcpSocket::Send(int fd, const uint8_t *buff, size_t length)
{
    auto ret = ::send(fd, buff, length, SOCKET_FLAG);
    if (ret < RET_OK) {
        CLOGE("Socket send error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
    }
    return ret;
}

ssize_t TcpSocket::Recv(int fd, uint8_t *buff, size_t length)
{
    size_t recvLen = 0;
    while (recvLen < length) {
        ssize_t len;
        int error;
        do {
            len = ::recv(fd, buff + recvLen, length - recvLen, SOCKET_FLAG);
            error = errno;
            if (stopReceive_) {
                return STOP_RECEIVE;
            }
        } while (len <= 0 && (error == EINTR || error == EAGAIN));

        if (len < RET_OK) {
            CLOGE("Socket recv error: errno = %{public}d, errmsg = %{public}s.", error, strerror(error));
            return RET_ERR;
        }

        recvLen += static_cast<size_t>(len);
    }

    CLOGI("Socket recv DONE!, size:%zu", recvLen);
    return recvLen;
}

void TcpSocket::Close()
{
    if (socket_ > INVALID_SOCKET) {
        ::close(socket_);
        socket_ = INVALID_SOCKET;
    }
}

void TcpSocket::Shutdown(int fd)
{
    if (fd > INVALID_SOCKET) {
        stopReceive_ = true;
        ::shutdown((fd), SHUT_RDWR);
        ::close(fd);
    }
}

bool TcpSocket::SetSendBufferSize(int size)
{
    int buffSize = size;
    if (setsockopt(socket_, SOL_SOCKET, SO_SNDBUF, &buffSize, sizeof(buffSize)) < RET_OK) {
        CLOGE("Socket SetSendBufferSize error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return false;
    }
    CLOGD("Socket SetSendBufferSize success.");
    return true;
}

bool TcpSocket::SetRecvBufferSize(int size)
{
    int buffSize = size;
    if (setsockopt(socket_, SOL_SOCKET, SO_RCVBUF, &buffSize, sizeof(buffSize)) < RET_OK) {
        CLOGE("Socket SetRecvBufferSize error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return false;
    }
    CLOGD("Socket SetRecvBufferSize success.");
    return true;
}

bool TcpSocket::SetLinger(bool active, int seconds)
{
    struct linger lgr{};
    lgr.l_onoff = active ? SOCKET_ON : SOCKET_OFF;
    lgr.l_linger = seconds;
    if (setsockopt(socket_, SOL_SOCKET, SO_LINGER, &lgr, sizeof(lgr)) < RET_OK) {
        CLOGE("Socket SetLinger error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return false;
    }
    CLOGD("Socket SetLinger success.");
    return true;
}

bool TcpSocket::SetKeepAlive()
{
    return SetKeepAlive(DEFAULT_VALUE, DEFAULT_VALUE, DEFAULT_VALUE);
}

bool TcpSocket::SetKeepAlive(unsigned idleTime, unsigned numProbes, unsigned probeInterval)
{
    int flag = SOCKET_ON;
    if (setsockopt(socket_, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)) < RET_OK) {
        CLOGE("Socket SetKeepAlive error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return false;
    }
    if (idleTime != DEFAULT_VALUE) {
        if (setsockopt(socket_, SOL_TCP, TCP_KEEPIDLE, &idleTime, sizeof(idleTime)) < RET_OK) {
            CLOGE("Socket SetKeepIDLE error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
            return false;
        }
    }
    if (numProbes != DEFAULT_VALUE) {
        if (setsockopt(socket_, SOL_TCP, TCP_KEEPCNT, &numProbes, sizeof(numProbes)) < RET_OK) {
            CLOGE("Socket SetKeepCNT error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
            return false;
        }
    }
    if (probeInterval != DEFAULT_VALUE) {
        if (setsockopt(socket_, SOL_TCP, TCP_KEEPINTVL, &probeInterval, sizeof(probeInterval)) < RET_OK) {
            CLOGE("Socket SetKeepINTVL error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
            return false;
        }
    }
    CLOGD("Socket SetKeepAlive success.");
    return true;
}

bool TcpSocket::SetReuseAddr()
{
    int flag = SOCKET_ON;
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < RET_OK) {
        CLOGE("Socket SetReuseAddr error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return false;
    }
    CLOGD("Socket SetReuseAddr success.");
    return true;
}

bool TcpSocket::SetIPTOS(int fd)
{
    /*
    数据包级别设置成VO级别
    Wi-Fi组织提出了一种无线QoS协议Wi-Fi多媒体标准WMM（Wi-Fi Multimedia）,
    将数据报文按照优先级从高到低分为4个接入类AC（Access Category） :
    AC_VO(语音)、AC_VI(视频)、AC_BE(尽力而为)、AC_BK(背景),
    高优先级的AC占用信道的机会大于低优先级的AC
    */
    int tos = IPTOS_LOWDELAY;
    if (setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos)) < RET_OK) {
        CLOGE("Socket IP_TOS error: errno = %{public}d, errmsg = %{public}s.", errno, strerror(errno));
        return false;
    }
    CLOGD("Socket IP_TOS success.");
    return true;
}
} // namespace CastEngineService
} // namespace CastEngine
} // namespace OHOS