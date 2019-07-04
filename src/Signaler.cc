#include <signal.h>
#include <unistd.h>
#include <functional>
#include <mutex>
#include <memory>
#include "EventLoop.h"
#include "Channel.h"
#include "Signaler.h"
#include "Socket.h"
#include "SockOps.h"
#include "LogStream.h"
#include "decls.h"

using namespace Angel;
using namespace std::placeholders;

namespace Angel{
    // 保护__signalerPtr，使之正确初始化
    std::mutex _SYNC_INIT_LOCK;
    Angel::Signaler *__signalerPtr = nullptr;

    static int _signalFd = -1;
}

Signaler::Signaler(EventLoop *loop)
    : _loop(loop),
    _sigChannel(new Channel(loop))
{
    if (_signalFd != -1)
        LOG_FATAL << "Only have one Signaler in one process";
    SockOps::socketpair(_pairFd);
    _sigChannel->setFd(_pairFd[0]);
    _signalFd = _pairFd[1];
    _sigChannel->setEventReadCb([this]{ this->sigCatch(); });
    LOG_INFO << "[Signaler::ctor]";
}

Signaler::~Signaler()
{
    LOG_INFO << "[Signaler::dtor]";
}

void Signaler::start()
{
    _loop->addChannel(_sigChannel);
}

void Signaler::sigHandler(int signo)
{
    ssize_t n = write(_signalFd,
                      reinterpret_cast<void *>(&signo), 1);
    LOG_INFO << "Sig" << sys_signame[signo] << " is triggered";
    if (n != 1)
        LOG_ERROR << "write " << n << " bytes instead of 1";
}

void Signaler::add(int signo, const SignalerCallback _cb)
{
    _loop->runInLoop(
            [this, signo, _cb]{ this->addInLoop(signo, _cb); });
}

void Signaler::cancel(int signo)
{
    _loop->runInLoop(
            [this, signo]{ this->cancelInLoop(signo); });
}

// if _cb == nullptr, ignore signo
void Signaler::addInLoop(int signo, const SignalerCallback _cb)
{
    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    if (_cb) {
        sa.sa_handler = &sigHandler;
        auto it = std::pair<int, SignalerCallback>(signo, std::move(_cb));
        _sigCallbackMaps.insert(it);
    } else {
        sa.sa_handler = SIG_IGN;
    }
    // 重启被信号中断的Syscall
    sa.sa_flags |= SA_RESTART;
    // 建立SigHandler之前屏蔽所有信号
    sigfillset(&sa.sa_mask);
    if (sigaction(signo, &sa, nullptr) < 0)
        LOG_ERROR << "sigaction: " << strerrno();
}

// 恢复信号signo的默认语义
void Signaler::cancelInLoop(int signo)
{
    struct sigaction sa;
    bzero(&sa, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    _sigCallbackMaps.erase(signo);
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    if (sigaction(signo, &sa, nullptr) < 0)
        LOG_ERROR << "sigaction: " << strerrno();
}

void Signaler::sigCatch()
{
    static char buf[1024];
    bzero(buf, sizeof(buf));
    ssize_t n = read(_sigChannel->fd(), buf, sizeof(buf));
    if (n < 0)
        LOG_ERROR << "read(): " << strerrno();
    for (int i = 0; i < n; i++) {
        auto it = _sigCallbackMaps.find(buf[i]);
        if (it != _sigCallbackMaps.end())
            it->second();
    }
}

void Angel::addSignal(int signo, const SignalerCallback _cb)
{
    if (__signalerPtr)
        __signalerPtr->add(signo, std::move(_cb));
}

void Angel::cancelSignal(int signo)
{
    if (__signalerPtr)
        __signalerPtr->cancel(signo);
}
