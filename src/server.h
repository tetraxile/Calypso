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

    struct packetBase {
        s8 type;
    };

    struct packetSettings {
        s8 type;
        bool moonRefresh;
        bool alwasyManualCutscenes;
        bool disableMoonLock;
        bool alwaysAllowCheckpoints;
        bool disableSave;
        bool disableHud;
        bool disableMusic;
    };

    struct packetScript {
        s8 type;
        s8 lenName;
        char name[0xff];
        s8 lenScript;
        char script[];

    };

    void handleSettingsPacket(packetSettings* packet);
    void handleScriptPacket(packetScript* packet);

    bool moonRefresh = false;
    bool alwasyManualCutscenes = false; 
    bool disableMoonLock = false;
    bool alwaysAllowCheckpoints = false;
    bool disableSave = false;
    bool disableHud = false;
    bool disableMusic = false;


public:
    Server() = default;

    void init(sead::Heap* heap);
    void threadRecv();
    s32 connect(const char* serverIP, u16 port);

    static void log(const char* fmt, ...);

    bool getMoonRefresh() { return moonRefresh; }
    bool getAlwasyManualCutscenes() { return alwasyManualCutscenes; }
    bool getDisableMoonLock() { return disableMoonLock; }
    bool getAlwaysAllowCheckpoints() { return alwaysAllowCheckpoints; }
    bool getDisableSave() { return disableSave; }
    bool getDisableHud() { return disableHud; }
    bool getDisableMusic() { return disableMusic; }
};

}

#endif
