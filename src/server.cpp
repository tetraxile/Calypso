#include "server.h"

#include "heap/seadDisposer.h"
#include "hk/diag/diag.h"
#include "hk/hook/Trampoline.h"
#include "nn/fs/fs_files.h"
#include "nn/fs/fs_types.h"

#include <cstdio>
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>

#include <nn/fs.h>
#include <nn/nifm.h>
#include <nn/socket.h>

#include <heap/seadHeap.h>
#include <heap/seadHeapMgr.h>
#include <math/seadMathCalcCommon.h>
#include <prim/seadEndian.h>


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

void Server::threadRecv() {
    while (true) {
        handlePacket();
        nn::os::SleepThread(nn::TimeSpan::FromSeconds(1));
    }
}

void Server::handlePacket() {
    u8 header[cPacketHeaderSize];
    recvAll(header, cPacketHeaderSize);
    PacketType packetType = PacketType(header[0]);

    switch (packetType) {
    case PacketType::Script: {
        u8 scriptNameLen = header[1];
        u32 scriptLen = sead::Endian::swapU32(*(u32*)&header[4]);

        char scriptName[0x101];
        recvAll((u8*)scriptName, 0xff);
        scriptName[0x100] = '\0';

        log("received packet Script: name = %s, len = %d (%d)", scriptName, scriptLen);

        char filePath[0x200];
        snprintf(filePath, sizeof(filePath), "sd:/Calypso/scripts/%s", scriptName);

        // TODO: replace these aborts with proper logs
        nn::Result r;

        nn::fs::DirectoryEntryType entryType;
        r = nn::fs::GetEntryType(&entryType, filePath);
        // HK_ABORT_UNLESS_R(hk::Result(r.GetInnerValueForDebug()));

        if (entryType == nn::fs::DirectoryEntryType_File) {
            r = nn::fs::DeleteFile(filePath);
            HK_ABORT_UNLESS_R(hk::Result(r.GetInnerValueForDebug()));
        }

        r = nn::fs::CreateFile(filePath, scriptLen);
        HK_ABORT_UNLESS_R(hk::Result(r.GetInnerValueForDebug()));

        nn::fs::FileHandle fileHandle;
        r = nn::fs::OpenFile(&fileHandle, filePath, nn::fs::OpenMode_Write);
        HK_ABORT_UNLESS_R(hk::Result(r.GetInnerValueForDebug()));

        u8 chunkBuf[0x10000];
        memset(chunkBuf, 0, sizeof(chunkBuf));
        s32 totalWritten = 0;
        while (totalWritten < scriptLen) {
            s32 remaining = scriptLen - totalWritten;
            s32 chunkLen = recvAll(chunkBuf, sead::Mathf::min(remaining, sizeof(chunkBuf)));
            // log("write: offset %#x, chunk len %#x", totalWritten, chunkLen);
            r = nn::fs::WriteFile(fileHandle, totalWritten, chunkBuf, chunkLen, nn::fs::WriteOption::CreateOption(nn::fs::WriteOptionFlag_Flush));
            HK_ABORT_UNLESS_R(hk::Result(r.GetInnerValueForDebug()));
            totalWritten += chunkLen;
        }

        nn::fs::CloseFile(fileHandle);

        log("wrote %d bytes to file '%s'", totalWritten, filePath);

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
