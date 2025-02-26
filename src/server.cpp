#include "server.h"

#include "heap/seadDisposer.h"
#include "hk/diag/diag.h"
#include "hk/hook/Trampoline.h"

#include <cstdio>
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>

#include <heap/seadHeap.h>
#include <heap/seadHeapMgr.h>
#include <math/seadMathCalcCommon.h>
#include <prim/seadEndian.h>

#include <nn/nifm.h>
#include <nn/socket.h>

HkTrampoline<void> disableSocketInit = hk::hook::trampoline([]() -> void {});

static constexpr int socketPoolSize = 0x600000;
static constexpr int socketAllocPoolSize = 0x20000;
char socketPool[socketPoolSize + socketAllocPoolSize] __attribute__((aligned(0x1000)));

namespace tas {

SEAD_SINGLETON_DISPOSER_IMPL(Server);

void Server::init(sead::Heap* heap) {
    mHeap = heap;

    sead::ScopedCurrentHeapSetter heapSetter(mHeap);

    al::FunctorV0M functor(this, &Server::threadRecv);
    mRecvThread = new al::AsyncFunctorThread("Recv Thread", functor, 0, 0x20000, {});

    nn::Result result = nn::nifm::Initialize();
    HK_ABORT_UNLESS(result.IsSuccess(), "nifm init failed", 0);
    
    result = nn::socket::Initialize(socketPool, socketPoolSize, socketAllocPoolSize, 0xE);

    nn::nifm::SubmitNetworkRequestAndWait();

    if (!nn::nifm::IsNetworkAvailable()) {
        HK_ABORT("network unavailable", 0);
    }

    disableSocketInit.installAtSym<"_ZN2nn6socket10InitializeEPvmmi">();
}

void Server::threadRecv() {
    while (true) {
        handlePacket();
        nn::os::SleepThread(nn::TimeSpan::FromSeconds(1));
    }
}

s32 Server::recvAll(u8* recvBuf, s32 remaining) {
    s32 totalReceived = 0;
    do {
        s32 recvLen = nn::socket::Recv(mSockFd, recvBuf, remaining, 0);
        if (recvLen <= 0) {
            return recvLen;
        }
        remaining -= recvLen;
        recvBuf += recvLen;
        totalReceived += recvLen;
    } while (remaining > 0);

    return totalReceived;
}

void Server::handlePacket() {
    u8 header[cPacketHeaderSize];
    recvAll(header, cPacketHeaderSize);
    PacketType packetType = PacketType(header[0]);

    switch (packetType) {
    case PacketType::ScriptInfo: {
        u8 scriptNameLen = header[1];
        char scriptName[0x100];
        recvAll((u8*)scriptName, scriptNameLen);

        log("received packet ScriptInfo: %s", scriptName);

        break;
    }
    case PacketType::ScriptData: {
        u32 scriptLen = sead::Endian::swapU32(*(u32*)&header[4]);
        u8 chunkBuf[0x1000];
        s32 totalReceived = 0;
        while (totalReceived < scriptLen) {
            s32 remaining = scriptLen - totalReceived;
            s32 chunkLen = recvAll(chunkBuf, sead::Mathf::min(remaining, sizeof(chunkBuf)));
            totalReceived += chunkLen;
        }
        log("received packet ScriptData: expected = %d, len = %d", scriptLen, totalReceived);

        break;
    }
    case PacketType::None:
    default:
        break;
    }
}

s32 Server::connect(const char* serverIP, u16 port) {
    if (mState == State::CONNECTED) return 0;
    
    in_addr hostAddress = {0};
    sockaddr_in serverAddr = {0};
    
    // create socket
    if ((mSockFd = nn::socket::Socket(AF_INET, SOCK_STREAM, 0)) < 0) return nn::socket::GetLastErrno();

    // configure server to connect to
    nn::socket::InetAton(serverIP, &hostAddress);
    serverAddr.sin_addr = hostAddress;
    serverAddr.sin_family = nn::socket::InetHtons(AF_INET);
    serverAddr.sin_port = nn::socket::InetHtons(port);

    // connect to server
    nn::Result result = nn::socket::Connect(mSockFd, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result.IsFailure()) return nn::socket::GetLastErrno();

    mState = State::CONNECTED;

    // send message to server
    char message[] = "connected!";
    s32 r = nn::socket::Send(mSockFd, message, strlen(message), 0);
    if (r < 0) return nn::socket::GetLastErrno();

    mRecvThread->start();

    return 0;
}

void Server::log(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char message[0x400];
    vsnprintf(message, sizeof(message), fmt, args);

    Server* server = instance();
    s32 r = nn::socket::Send(server->mSockFd, message, strlen(message), 0);

    va_end(args);
}

}  // namespace tas
