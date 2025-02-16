#include "hk/Result.h"
#include "hk/diag/diag.h"
#include "hk/hook/Trampoline.h"
#include "hk/util/Context.h"

#include <netdb.h>
#include <netinet/in.h>

#include <nn/socket.h>
#include <nn/nifm.h>

#include "game/System/GameSystem.h"

using namespace hk;

static constexpr int poolSize = 0x600000;
static constexpr int allocPoolSize = 0x20000;
char socketPool[poolSize + allocPoolSize] __attribute__((aligned(0x1000)));

HkTrampoline<void> disableSocketInit = hk::hook::trampoline([]() -> void {});

HkTrampoline<void, GameSystem*> gameSystemInit = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    const char* serverIP = "192.168.1.13";
    u16 port = 8171;

    in_addr hostAddress = {0};
    sockaddr_in serverAddr = {0};

    nn::Result result = nn::nifm::Initialize();
    HK_ABORT_UNLESS(result.IsSuccess(), "nifm init failed", 0);
    
    result = nn::socket::Initialize(socketPool, poolSize, allocPoolSize, 0xE);

    nn::nifm::SubmitNetworkRequestAndWait();

    if (!nn::nifm::IsNetworkAvailable()) {
        HK_ABORT("network unavailable", 0);
    }

    // create socket
    s32 sockfd;
    if ((sockfd = nn::socket::Socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        HK_ABORT("socket ctor failed", 0);
    }

    nn::socket::InetAton(serverIP, &hostAddress);
    
    // configure server to connect to
    serverAddr.sin_addr = hostAddress;
    serverAddr.sin_family = nn::socket::InetHtons(AF_INET);
    serverAddr.sin_port = nn::socket::InetHtons(port);

    // connect to server
    result = nn::socket::Connect(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr));
    s32 r = nn::socket::GetLastErrno();
    const char* s = strerror(r);
    HK_ABORT_UNLESS(result.IsSuccess(), "connect failed (%s)", s);

    char message[0x400] = "hi";
    r = nn::socket::Send(sockfd, message, strlen(message), 0);
    HK_ABORT_UNLESS(r > -1, "send failed", 0);

    // 
    disableSocketInit.installAtSym<"_ZN2nn6socket10InitializeEPvmmi">();

    gameSystemInit.orig(gameSystem);
});

extern "C" void hkMain() {
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
}
