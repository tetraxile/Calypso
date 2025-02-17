#include "hk/Result.h"
#include "hk/diag/diag.h"
#include "hk/hook/Trampoline.h"
#include "hk/util/Context.h"

#include "server.h"

#include "game/System/GameSystem.h"

using namespace hk;

HkTrampoline<void, GameSystem*> gameSystemInit = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    Server& server = Server::instance();
    server.init();

    server.connect("192.168.1.16", 8171);

    gameSystemInit.orig(gameSystem);
});

extern "C" void hkMain() {
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
}
