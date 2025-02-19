#ifndef SERVER_H
#define SERVER_H

#include <basis/seadTypes.h>
#include <heap/seadExpHeap.h>

#include <nn/os.h>

#include "al/Library/Thread/AsyncFunctorThread.h"

namespace tas {

class Server {
    SEAD_SINGLETON_DISPOSER(Server);

private:
    enum class State {
        UNINITIALIZED,
        DISCONNECTED,
        CONNECTED,
    };

    sead::Heap* mHeap = nullptr;
    al::AsyncFunctorThread* mRecvThread = nullptr;
    void* mThreadStack = nullptr;
    s32 mSockFd = -1;
    State mState = State::UNINITIALIZED;

public:
    Server() = default;

    void init(sead::Heap* heap);
    void threadFunc();
    s32 connect(const char* serverIP, u16 port);

    static void log(const char* fmt, ...);
};

}

#endif
