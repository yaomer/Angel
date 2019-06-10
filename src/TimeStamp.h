#ifndef _ANGEL_TIMESTAMP_H
#define _ANGEL_TIMESTAMP_H

#include <cinttypes>

namespace Angel {

class TimeStamp {
public:
    enum Type {
        GMT_TIME,
        LOCAL_TIME,
    };
    int64_t time() { _ms = now(); return _ms; }
    static int64_t now();
    static const char *timeStr(int option, char *buf, size_t len);
private:
    int64_t _ms = 0;
};
}

#endif // _ANGEL_TIMESTAMP_H
