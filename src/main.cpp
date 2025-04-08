#include "menu.h"
#include "server.h"
#include "tas.h"

#include <hk/Result.h>
#include <hk/diag/diag.h>
#include <hk/gfx/DebugRenderer.h>
#include <hk/hook/Trampoline.h>
#include <hk/util/Context.h>

#include <agl/common/aglDrawContext.h>
#include <nn/fs.h>
#include <nn/hid.h>
#include <sead/controller/seadControllerAddon.h>
#include <sead/controller/seadControllerMgr.h>
#include <sead/filedevice/seadFileDeviceMgr.h>
#include <sead/heap/seadHeap.h>
#include <sead/heap/seadHeapMgr.h>
#include <sead/math/seadVector.h>

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/NpadController.h"
#include "al/Library/Memory/HeapUtil.h"
#include "game/System/GameSystem.h"

using namespace hk;

namespace cly {
bool gIsInitialized = false;

sead::Heap* initializeHeap() {
	return sead::ExpHeap::create(1_MB, "CalypsoHeap", al::getStationedHeap(), 8, sead::Heap::cHeapDirection_Forward, false);
}

void endInit() {
	gIsInitialized = true;
}
} // namespace cly

HkTrampoline<void, sead::FileDeviceMgr*> fileDeviceMgrHook = hk::hook::trampoline([](sead::FileDeviceMgr* fileDeviceMgr) -> void {
	fileDeviceMgrHook.orig(fileDeviceMgr);

	fileDeviceMgr->mMountedSd = nn::fs::MountSdCardForDebug("sd") == 0;
});

HkTrampoline<void, GameSystem*> gameSystemInit = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
	sead::Heap* heap = cly::initializeHeap();
	cly::Menu* menu = cly::Menu::createInstance(heap);
	menu->init(heap);

	cly::Server* server = cly::Server::createInstance(heap);
	server->init(heap);
	server->connect("192.168.1.215", 8171);

	cly::tas::System* system = cly::tas::System::createInstance(heap);
	system->init(heap);

	cly::endInit();

	gameSystemInit.orig(gameSystem);
});

HkTrampoline<void, GameSystem*> drawMainHook = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
	drawMainHook.orig(gameSystem);

	cly::Menu::instance()->draw();
});

bool isPressedA = false;

HkTrampoline<void, al::NpadController*> inputHook = hk::hook::trampoline([](al::NpadController* controller) -> void {
	inputHook.orig(controller);

	if (!cly::gIsInitialized) return;

	if (controller->mControllerMode == -1 || controller->mControllerMode == 0) {
		// player 1
		controller->mPadHold.changeBit(sead::Controller::cPadIdx_A, isPressedA);
		isPressedA = isPressedA ^ 1;
		// cly::Menu::log(
		// 	"%d %d %d %d %08x %d", controller->mIsConnected ? 1 : 0, controller->mControllerMode, controller->mNpadId, controller->mNpadStyleTag,
		// 	controller->mPadHold, controller->mSamplingNumber
		// );
	} else if (controller->mControllerMode == 1) {
		// player 2
	}
});

// HkTrampoline<void, sead::Vector2f*, const al::CameraPoser*> cameraRotateHook =
// hk::hook::trampoline([](sead::Vector2f* out, const al::CameraPoser* poser) -> void {
//     out->x = 0.0f;
//     out->y = 0.0f;
// });

extern "C" void hkMain() {
	hk::gfx::DebugRenderer::instance()->installHooks();
	gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
	drawMainHook.installAtSym<"_ZN10GameSystem8drawMainEv">();
	inputHook.installAtSym<"_ZN2al14NpadController9calcImpl_Ev">();
	fileDeviceMgrHook.installAtSym<"_ZN4sead13FileDeviceMgrC1Ev">();
}
