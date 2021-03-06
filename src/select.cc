#include "config.h"

#ifdef ANGEL_HAVE_SELECT

#include "evloop.h"
#include "select.h"
#include "logger.h"

using namespace angel;
using namespace angel::util;

select_base_t::select_base_t()
{
    FD_ZERO(&rdset);
    FD_ZERO(&wrset);
    fds.reserve(fds_init_size);
    maxfd = -1;
    set_name("select");
}

void select_base_t::add(int fd, event events)
{
    fds.push_back(fd);
    indexs.emplace(fd, fds.size() - 1);
    change(fd, events);
    maxfd = std::max(maxfd, fd, std::greater<int>());
}

void select_base_t::change(int fd, event events)
{
    // 要修改关联到fd上的事件，必须先从rdset和wrset中
    // 清除掉原先注册的fd，然后再重新注册，不然就可能会
    // 出现重复注册同一个fd
    FD_CLR(fd, &rdset);
    FD_CLR(fd, &wrset);
    if (is_enum_true(events & event::read))
        FD_SET(fd, &rdset);
    if (is_enum_true(events & event::write))
        FD_SET(fd, &wrset);
}

void select_base_t::remove(int fd, event events)
{
    UNUSED(events);
    auto it = indexs.find(fd);
    size_t end = fds.size() - 1;
    std::swap(fds[it->second], fds[end]);
    fds.pop_back();
    indexs.erase(fd);
    change(fd, event());
    if (fd == maxfd)
        maxfd--;
}

int select_base_t::wait(evloop *loop, int64_t timeout)
{
    struct timeval tv;
    // 我们不能直接将rdset和wrset传递给select()，因为select()会改写它们
    // 以存放活跃的fd，我们需要传递给它一个副本
    fd_set rdset1, wrset1, errset;
    FD_ZERO(&rdset1);
    FD_ZERO(&wrset1);
    FD_ZERO(&errset);
    rdset1 = rdset;
    wrset1 = wrset;
    if (timeout > 0) {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
    }
    int nevents = select(maxfd + 1, &rdset1, &wrset1, &errset,
            timeout > 0 ? &tv : nullptr);
    int rets = nevents;
    if (nevents > 0) {
        for (auto& it : fds) {
            event revs{};
            if (FD_ISSET(it, &rdset1))
                revs |= event::read;
            if (FD_ISSET(it, &wrset1))
                revs |= event::write;
            if (FD_ISSET(it, &errset))
                revs |= event::error;
            auto chl = loop->search_channel(it);
            chl->set_trigger_events(revs);
            loop->fill_active_channel(chl);
            if (angel::get_enum_value(revs) && --nevents == 0)
                break;
        }
    } else if (nevents < 0) {
        if (errno != EINTR)
            log_error("select: %s", strerrno());
    }
    return rets;
}

#endif
