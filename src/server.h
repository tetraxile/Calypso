#ifndef SERVER_H
#define SERVER_H

#include <basis/seadTypes.h>

class Server {
private:
    s32 mSockFd = -1;

public:
    static Server& instance() {
        static Server s;
        return s;
    }

    void init();
    s32 connect(const char* serverIP, u16 port);
};

#endif