#include "hk/Result.h"
#include "hk/diag/diag.h"
#include "hk/hook/Trampoline.h"
#include "hk/util/Context.h"

#include <agl/common/aglDrawContext.h>

#include <gfx/seadTextWriter.h>
#include <gfx/seadViewport.h>
#include <heap/seadHeap.h>
#include <heap/seadHeapMgr.h>

#include "al/Library/Memory/HeapUtil.h"

#include "game/System/GameSystem.h"

#include "server.h"
#include "menu.h"

using namespace hk;

namespace tas {
sead::Heap* initializeHeap() {
    return sead::ExpHeap::create(0x100000, "CalypsoHeap", al::getStationedHeap(), 8, sead::Heap::cHeapDirection_Forward, false);
}
}

HkTrampoline<void, GameSystem*> gameSystemInit = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    sead::Heap* heap = tas::initializeHeap();
    tas::Server* server = tas::Server::createInstance(heap);
    server->init(heap);

    tas::Menu::initFontMgr();
    tas::Menu* menu = tas::Menu::createInstance(heap);
    menu->init(heap);

    gameSystemInit.orig(gameSystem);
});


HkTrampoline<void, GameSystem*> drawMainLoop = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    drawMainLoop.orig(gameSystem);

    tas::Menu::instance()->draw();
});

extern "C" void hkMain() {
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();

    drawMainLoop.installAtSym<"_ZN10GameSystem8drawMainEv">();
}
