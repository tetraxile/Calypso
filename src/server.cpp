#include "server.h"

#include "hk/diag/diag.h"
#include "hk/hook/Trampoline.h"

#include <netdb.h>
#include <netinet/in.h>

#include <nn/socket.h>
#include <nn/nifm.h>


static constexpr int poolSize = 0x600000;
static constexpr int allocPoolSize = 0x20000;
char socketPool[poolSize + allocPoolSize] __attribute__((aligned(0x1000)));

HkTrampoline<void> disableSocketInit = hk::hook::trampoline([]() -> void {});

void Server::init() {
    nn::Result result = nn::nifm::Initialize();
    HK_ABORT_UNLESS(result.IsSuccess(), "nifm init failed", 0);
    
    result = nn::socket::Initialize(socketPool, poolSize, allocPoolSize, 0xE);

    nn::nifm::SubmitNetworkRequestAndWait();

    if (!nn::nifm::IsNetworkAvailable()) {
        HK_ABORT("network unavailable", 0);
    }

    disableSocketInit.installAtSym<"_ZN2nn6socket10InitializeEPvmmi">();
}

s32 Server::connect(const char* serverIP, u16 port) {
    in_addr hostAddress = {0};
    sockaddr_in serverAddr = {0};
    
    // create socket
    if ((mSockFd = nn::socket::Socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        HK_ABORT("socket ctor failed", 0);
    }

    // configure server to connect to
    nn::socket::InetAton(serverIP, &hostAddress);
    serverAddr.sin_addr = hostAddress;
    serverAddr.sin_family = nn::socket::InetHtons(AF_INET);
    serverAddr.sin_port = nn::socket::InetHtons(port);

    // connect to server
    nn::Result result = nn::socket::Connect(mSockFd, (sockaddr*)&serverAddr, sizeof(serverAddr));
    s32 r = nn::socket::GetLastErrno();
    const char* s = strerror(r);
    HK_ABORT_UNLESS(result.IsSuccess(), "connect failed (%s)", s);

    // send message to server
    char message[0x400] = "hi";
    r = nn::socket::Send(mSockFd, message, strlen(message), 0);
    HK_ABORT_UNLESS(r > -1, "send failed", 0);

    return 0;
}