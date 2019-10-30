#ifndef _ANGEL_SELECT_H
#define _ANGEL_SELECT_H

#include <sys/select.h>
#include <vector>
#include <map>

#include "Poller.h"
#include "noncopyable.h"

namespace Angel {

class Select : public Poller, noncopyable {
public:
    Select();
    ~Select() {  };
    int wait(EventLoop *loop, int64_t timeout);
    void add(int fd, int events);
    void change(int fd, int events);
    void remove(int fd, int events);
    static const size_t fds_init_size = 64;
private:
    fd_set _rdset;
    fd_set _wrset;
    std::vector<int> _fds;
    // <fd, index>, index for _fds
    std::map<int, int> _indexs;
    int _maxFd;
};
}

#endif // _ANGEL_SELECT_H
