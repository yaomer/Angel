#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "SocketOps.h"
#include "LogStream.h"

using namespace Angel;
using std::placeholders::_1;

TcpServer::TcpServer(EventLoop *loop, InetAddr& listenAddr)
    : _loop(loop),
    _acceptor(new Acceptor(loop, listenAddr)),
    _connId(1)
{
    _acceptor->setNewConnectionCb(
            std::bind(&TcpServer::newConnection, this, _1));
    LOG_INFO << "[TcpServer::ctor]";
}

TcpServer::~TcpServer()
{
    LOG_INFO << "[TcpServer::dtor]";
}

size_t TcpServer::getId()
{
    size_t id;
    if (_freeIdList.empty()) {
        id = _connId++;
    } else {
        id = *_freeIdList.begin();
        _freeIdList.erase(_freeIdList.begin());
    }
    return id;
}

void TcpServer::putId(size_t id)
{
    _freeIdList.insert(id);
}

EventLoop *TcpServer::getNextLoop()
{
    return _loop;
}

void TcpServer::newConnection(int fd)
{
    size_t id = getId();
    EventLoop *ioLoop = getNextLoop();
    InetAddr localAddr = SocketOps::getLocalAddr(fd);
    InetAddr peerAddr = SocketOps::getPeerAddr(fd);
    TcpConnectionPtr conn(new TcpConnection(id, ioLoop, fd, localAddr, peerAddr));
    if (_connectionCb) _connectionCb(conn);
    // conn->setConnectionCb(_connectionCb);
    conn->setMessageCb(_messageCb);
    conn->setCloseCb(
            std::bind(&TcpServer::removeConnection, this, _1));
    _connectionMaps[id] = std::move(conn);
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    putId(conn->id());
    conn->getLoop()->removeChannel(conn->getChannel());
    _connectionMaps.erase(conn->id());
}

void TcpServer::start()
{
    _acceptor->listen();
}

void TcpServer::quit()
{
    _loop->quit();
    _loop->wakeup();
}
