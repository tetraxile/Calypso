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

#include <nn/nifm.h>
#include <nn/socket.h>

#include "al/Library/Memory/HeapUtil.h"

static constexpr int poolSize = 0x600000;
static constexpr int allocPoolSize = 0x20000;
char socketPool[poolSize + allocPoolSize] __attribute__((aligned(0x1000)));

HkTrampoline<void> disableSocketInit = hk::hook::trampoline([]() -> void {});

SEAD_SINGLETON_DISPOSER_IMPL(Server);

sead::Heap* Server::initializeHeap() {
    return sead::ExpHeap::create(0x100000, "ClientHeap", al::getStationedHeap(), 8, sead::Heap::cHeapDirection_Forward, false);
}

void Server::init(sead::Heap* heap) {
    mHeap = heap;

    sead::ScopedCurrentHeapSetter heapSetter(mHeap);

    al::FunctorV0M functor(this, &Server::threadFunc);
    mRecvThread = new al::AsyncFunctorThread("Recv Thread", functor, 0, 0x20000, {});

    nn::Result result = nn::nifm::Initialize();
    HK_ABORT_UNLESS(result.IsSuccess(), "nifm init failed", 0);
    
    result = nn::socket::Initialize(socketPool, poolSize, allocPoolSize, 0xE);

    nn::nifm::SubmitNetworkRequestAndWait();

    if (!nn::nifm::IsNetworkAvailable()) {
        HK_ABORT("network unavailable", 0);
    }

    disableSocketInit.installAtSym<"_ZN2nn6socket10InitializeEPvmmi">();
}

void Server::threadFunc() {
    Server::log("thread\n");
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
    snprintf(message, sizeof(message), fmt, args);

    Server* server = instance();
    s32 r = nn::socket::Send(server->mSockFd, message, strlen(message), 0);

    va_end(args);
}
