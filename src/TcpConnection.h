#ifndef _ANGEL_TCPCONNECTION_H
#define _ANGEL_TCPCONNECTION_H

#include <functional>
#include <memory>
#include <atomic>
#include "Channel.h"
#include "Buffer.h"
#include "InetAddr.h"
#include "Socket.h"
#include "noncopyable.h"
#include "decls.h"

#include <boost/any.hpp>

namespace Angel {

class EventLoop;

class TcpConnection : noncopyable,
    public std::enable_shared_from_this<TcpConnection> {
public:
    TcpConnection(size_t id,
                  EventLoop *loop, 
                  int sockfd, 
                  InetAddr localAddr,
                  InetAddr peerAddr);
    ~TcpConnection();
    enum State {
        CONNECTING,
        CONNECTED,
        CLOSING,
        CLOSED,
    };
    size_t id() const { return _id; }
    void send(const char *s);
    void send(const std::string& s);
    void send(const char *s, size_t len);
    void send(const void *v, size_t len);
    EventLoop *getLoop() { return _loop; }
    ChannelPtr& getChannel() { return _channel; }
    InetAddr& localAddr() { return _localAddr; }
    InetAddr& peerAddr() { return _peerAddr; }
    void setState(char state) { _state = state; }
    bool isConnected() { return _state == CONNECTED; }
    boost::any& getContext() { return _context; }
    void setContext(boost::any context) { _context = std::move(context); }
    void setTimeoutTimerId(size_t id) { _timeoutTimerId = id; }
    void setConnTimeout(int64_t timeout) { _connTimeout = timeout; }
    void close();
    void setConnectionCb(const ConnectionCallback _cb)
    { _connectionCb = std::move(_cb); }
    void setMessageCb(const MessageCallback _cb)
    { _messageCb = std::move(_cb); }
    void setWriteCompleteCb(const WriteCompleteCallback _cb)
    { _writeCompleteCb = std::move(_cb); }
    void setCloseCb(const CloseCallback _cb)
    { _closeCb = std::move(_cb); }
private:
    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();
    void sendInLoop(const std::string& s);
    void updateTimeoutTimer();

    size_t _id;
    EventLoop *_loop;
    std::shared_ptr<Channel> _channel;
    std::unique_ptr<Socket> _socket;
    Buffer _input;
    Buffer _output;
    InetAddr _localAddr;
    InetAddr _peerAddr;
    boost::any _context;
    std::atomic_int8_t _state;
    size_t _timeoutTimerId;
    int64_t _connTimeout;
    ConnectionCallback _connectionCb;
    MessageCallback _messageCb;
    WriteCompleteCallback _writeCompleteCb;
    CloseCallback _closeCb;
    ErrorCallback _errorCb;
};

}

#endif // _ANGEL_TCPCONNECTION_H
