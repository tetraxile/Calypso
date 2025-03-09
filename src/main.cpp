#include "menu.h"
#include "server.h"

#include <hk/Result.h>
#include <hk/diag/diag.h>
#include <hk/gfx/DebugRenderer.h>
#include <hk/hook/Trampoline.h>
#include <hk/util/Context.h>

#include <agl/common/aglDrawContext.h>
#include <nn/fs.h>
#include <sead/controller/seadControllerAddon.h>
#include <sead/controller/seadControllerMgr.h>
#include <sead/filedevice/seadFileDeviceMgr.h>
#include <sead/heap/seadHeap.h>
#include <sead/heap/seadHeapMgr.h>
#include <sead/math/seadVector.h>

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/NpadController.h"
#include "al/Library/Controller/PadReplayFunction.h"
#include "al/Library/Memory/HeapUtil.h"
#include "game/System/GameSystem.h"

using namespace hk;

namespace tas {
sead::Heap* initializeHeap() {
	return sead::ExpHeap::create(0x100000, "CalypsoHeap", al::getStationedHeap(), 8, sead::Heap::cHeapDirection_Forward, false);
}
} // namespace tas

HkTrampoline<void, sead::FileDeviceMgr*> fileDeviceMgrHook = hk::hook::trampoline([](sead::FileDeviceMgr* fileDeviceMgr) -> void {
	fileDeviceMgrHook.orig(fileDeviceMgr);

	fileDeviceMgr->mMountedSd = nn::fs::MountSdCardForDebug("sd") == 0;
});

HkTrampoline<void, GameSystem*> gameSystemInit = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
	sead::Heap* heap = tas::initializeHeap();
	tas::Server* server = tas::Server::createInstance(heap);
	server->init(heap);

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

	// nn::hid::NpadStyleSet styleSet = nn::hid::GetNpadStyleSet(port);

	// nn::hid::NpadBaseState* state = nullptr;
	// if (styleSet.Test(int(nn::hid::NpadStyleTag::NpadStyleFullKey)))
	//     nn::hid::GetNpadStates((nn::hid::NpadFullKeyState*)state, 0x10, port);
	// else if (styleSet.Test(int(nn::hid::NpadStyleTag::NpadStyleHandheld)))
	//     nn::hid::GetNpadStates((nn::hid::NpadHandheldState*)state, 0x10, port);
	// else if (styleSet.Test(int(nn::hid::NpadStyleTag::NpadStyleJoyDual)))
	//     nn::hid::GetNpadStates((nn::hid::NpadJoyDualState*)state, 0x10, port);
	// else if (styleSet.Test(int(nn::hid::NpadStyleTag::NpadStyleJoyLeft)))
	//     nn::hid::GetNpadStates((nn::hid::NpadJoyLeftState*)state, 0x10, port);
	// else if (styleSet.Test(int(nn::hid::NpadStyleTag::NpadStyleJoyRight)))
	//     nn::hid::GetNpadStates((nn::hid::NpadJoyRightState*)state, 0x10, port);

	// if (!state) return;
});

// HkTrampoline<void, sead::Vector2f*, const al::CameraPoser*> cameraRotateHook =
// hk::hook::trampoline([](sead::Vector2f* out, const al::CameraPoser* poser) -> void {
//     out->x = 0.0f;
//     out->y = 0.0f;
// });

extern "C" void hkMain() {
	gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
	drawMainHook.installAtSym<"_ZN10GameSystem8drawMainEv">();
	inputHook.installAtSym<"_ZN4sead13ControllerMgr4calcEv">();
	fileDeviceMgrHook.installAtSym<"_ZN4sead13FileDeviceMgrC1Ev">();
	hk::gfx::DebugRenderer::instance()->installHooks();
}
