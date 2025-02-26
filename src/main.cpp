#include "hk/Result.h"
#include "hk/diag/diag.h"
#include "hk/hook/Trampoline.h"
#include "hk/util/Context.h"

#include <agl/common/aglDrawContext.h>

#include <controller/seadControllerAddon.h>
#include <controller/seadControllerMgr.h>
#include <gfx/seadTextWriter.h>
#include <gfx/seadViewport.h>
#include <heap/seadHeap.h>
#include <heap/seadHeapMgr.h>
#include <math/seadVector.h>

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/NpadController.h"
#include "al/Library/Controller/PadReplayFunction.h"
#include "al/Library/Memory/HeapUtil.h"

#include "game/System/GameSystem.h"
#include "game/Scene/StageScene.h"
#include "al/Library/Audio/AudioUtil.h"
#include "game/Layout/MapMini.h"
#include "System/GameDataHolderWriter.h"

#include "server.h"
#include "menu.h"

using namespace hk;

#define CUTSCENE_HOOK_CALLBACK(NAME)                                                                     \
HkTrampoline<bool, void*> NAME = hk::hook::trampoline([](void* thisPtr) -> bool {               \
    tas::Server* server = tas::Server::instance();                                              \
        if (server->getAlwasyManualCutscenes())                                                 \
            return true;                                                                        \
        return NAME.orig(thisPtr);                                                              \
});

CUTSCENE_HOOK_CALLBACK(RsDemoHook);
CUTSCENE_HOOK_CALLBACK(FirstDemoScenarioHook);
CUTSCENE_HOOK_CALLBACK(FirstDemoWorldHook);
CUTSCENE_HOOK_CALLBACK(FirstDemoMoonRockHook);
CUTSCENE_HOOK_CALLBACK(ShowDemoHackHook);


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

HkTrampoline<void, GameSystem*> drawMainHook = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    drawMainHook.orig(gameSystem);

    tas::Menu::instance()->draw();
});

HkTrampoline<void, sead::ControllerMgr*> inputHook = hk::hook::trampoline([](sead::ControllerMgr* mgr) -> void {
    s32 port = al::getMainControllerPort();

    inputHook.orig(mgr);

    tas::Menu* menu = tas::Menu::instance();
    if (menu) menu->handleInput(port);
});

// HkTrampoline<void, sead::Vector2f*, const al::CameraPoser*> cameraRotateHook = hk::hook::trampoline([](sead::Vector2f* out, const al::CameraPoser* poser) -> void {
//     out->x = 0.0f;
//     out->y = 0.0f;
// });

HkTrampoline<void, StageScene*> controlHook = hk::hook::trampoline([](StageScene* scene) -> void {
    tas::Server* server = tas::Server::instance();

    if (server->getDisableMusic()) {
        if (al::isPlayingBgm(scene)) {
            al::stopAllBgm(scene, 0);
        }
    }
    if (server->getDisableHud() && scene->stageSceneLayout->isWait()) {
        scene->stageSceneLayout->end();
        MapMini* compass = scene->stageSceneLayout->mMapMini;
        if (compass->mIsAlive) compass->end();
    }
    
    controlHook.orig(scene);
});

HkTrampoline<bool, StageScene*> saveHook = hk::hook::trampoline([](StageScene* scene) -> bool {
    tas::Server* server = tas::Server::instance();
    return server->getDisableSave() ? false : saveHook.orig(scene);
});

HkTrampoline<bool, void*> checkpointWarpHook = hk::hook::trampoline([](void* thisPtr) -> bool {
    tas::Server* server = tas::Server::instance();
    return server->getAlwaysAllowCheckpoints() ? true : checkpointWarpHook.orig(thisPtr);
});

class ShineInfo;
HkTrampoline<bool, GameDataHolderWriter*, ShineInfo*> greyShineRefreshHook = hk::hook::trampoline([](GameDataHolderWriter* writer, ShineInfo* shine) -> bool {
    tas::Server* server = tas::Server::instance();
    return server->getMoonRefresh() ? false : greyShineRefreshHook.orig(writer, shine);
});

HkTrampoline<void, GameDataHolderWriter*, ShineInfo*> shineRefreshHook = hk::hook::trampoline([](GameDataHolderWriter* writer, ShineInfo* shine) -> void {
    tas::Server* server = tas::Server::instance();

    char* moonRefreshText = "Calypso";

    ro::getMainModule()->writeRo(0x01832301, moonRefreshText, strlen(moonRefreshText)+1);

    if (!server->getMoonRefresh())
        shineRefreshHook.orig(writer, shine);
});

HkTrampoline<int, GameDataHolder*, bool*, int> disableMoonLockHook = hk::hook::trampoline([](GameDataHolder* holder, bool* isCrashList, int worldID) -> int {
    tas::Server* server = tas::Server::instance();
    return server->getDisableMoonLock() ? 0 : disableMoonLockHook.orig(holder, isCrashList, worldID);
});

extern "C" void hkMain() {
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();

    drawMainHook.installAtSym<"_ZN10GameSystem8drawMainEv">();

    inputHook.installAtSym<"_ZN4sead13ControllerMgr4calcEv">();

    controlHook.installAtSym<"_ZN10StageScene7controlEv">();
    saveHook.installAtSym<"_ZNK10StageScene12isEnableSaveEv">();
    checkpointWarpHook.installAtSym<"_ZNK9MapLayout22isEnableCheckpointWarpEv">();
    greyShineRefreshHook.installAtSym<"_ZN16GameDataFunction10isGotShineE22GameDataHolderAccessorPK9ShineInfo">();
    shineRefreshHook.installAtSym<"_ZN16GameDataFunction11setGotShineE20GameDataHolderWriterPK9ShineInfo">();
    disableMoonLockHook.installAtSym<"_ZNK14GameDataHolder18findUnlockShineNumEPbi">();

    RsDemoHook.installAtSym<"_ZN2rs11isFirstDemoEPKN2al5SceneE">();
    FirstDemoScenarioHook.installAtSym<"_ZN2rs30isFirstDemoScenarioStartCameraEPKN2al9LiveActorE">();
    FirstDemoWorldHook.installAtSym<"_ZN2rs27isFirstDemoWorldIntroCameraEPKN2al5SceneE">();
    FirstDemoMoonRockHook.installAtSym<"_ZNK12MoonRockData38isEnableShowDemoAfterOpenMoonRockFirstEv">();
    ShowDemoHackHook.installAtSym<"_ZNK18DemoStateHackFirst20isEnableShowHackDemoEv">();
    
}
