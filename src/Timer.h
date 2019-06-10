#ifndef _ANGEL_TIMER_H
#define _ANGEL_TIMER_H

#include <cinttypes>
#include <set>
#include "TimerTask.h"
#include "TimeStamp.h"

namespace Angel {

class TimerTaskCmp {
public:
    bool operator()(const std::unique_ptr<TimerTask>& lhs,
                    const std::unique_ptr<TimerTask>& rhs) const
    {
        return lhs.get()->timeout() < rhs.get()->timeout();
    }
};

class Timer {
public:
    void add(TimerTask *_task)
    {
        _task->setTimeout(_task->timeout() + TimeStamp::now()); 
        _timer.insert(std::unique_ptr<TimerTask>(_task));
    }
    // 取出最小定时器
    const TimerTask *get() { return _timer.cbegin()->get(); };
    // 删除最小定时器事件
    void pop() { _timer.erase(_timer.begin()); }
    // 返回最小超时值
    int64_t timeout()
    {
        int64_t timeval;
        if (!_timer.empty()) {
            timeval = llabs(get()->timeout() - TimeStamp::now());
        } else
            timeval = -1;
        return timeval == 0 ? 1 : timeval;
    }
    // 处理所有到期的定时事件
    void tick();
private:
    std::set<std::unique_ptr<TimerTask>, TimerTaskCmp> _timer;
};
}

#endif // _ANGEL_TIMER_H
