#ifndef _ANGEL_EVENTLOOPTHREAD_H
#define _ANGEL_EVENTLOOPTHREAD_H

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Angel {

class EventLoop;

// One-loop-per-thread
class EventLoopThread {
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop *getLoop();
    EventLoop *getAssertTrueLoop() { return _loop; }
    void quit();
private:
    void threadFunc();

    EventLoop *_loop;
    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _condVar;
};
}

#endif // _ANGEL_EVENTLOOPTHREAD_H
