#ifndef _ANGEL_CONNECTOR
#define _ANGEL_CONNECTOR

#include <memory>
#include <functional>

#include "InetAddr.h"
#include "Channel.h"
#include "noncopyable.h"
#include "decls.h"

namespace Angel {

class EventLoop;

class Connector : noncopyable {
public:
    Connector(EventLoop *loop, InetAddr& inetAddr)
        : _loop(loop),
        _peerAddr(inetAddr),
        _connectChannel(new Channel(loop)),
        _connected(false),
        _waitRetry(false)
    {
    }
    ~Connector() {  };
    void connect();
    bool isConnected() { return _connected; }
    int connfd() const { return _connectChannel->fd(); }
    void setNewConnectionCb(const NewConnectionCallback _cb)
    { _newConnectionCb = std::move(_cb); }

private:
    static const int retry_interval = 3;

    void connecting(int fd);
    void connected(int fd);
    void check(int fd);
    void retry(int fd);

    EventLoop *_loop;
    InetAddr _peerAddr;
    std::shared_ptr<Channel> _connectChannel;
    NewConnectionCallback _newConnectionCb;
    bool _connected;
    bool _waitRetry;
};
}

#endif // _ANGEL_CONNECTOR
