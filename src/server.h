#ifndef SERVER_H
#define SERVER_H

#include <basis/seadTypes.h>

class Server {
private:
    enum class State {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
    };

    s32 mSockFd = -1;
    State mState = State::DISCONNECTED;

public:
    static Server& instance() {
        static Server s;
        return s;
    }

    void init();
    s32 connect(const char* serverIP, u16 port);

    static void log(const char* fmt, ...);
};

#endif
