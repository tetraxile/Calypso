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
#include "al/Library/Scene/Scene.h"
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

	cly::tas::Pauser* pauser = cly::tas::Pauser::createInstance(heap);

	cly::Menu* menu = cly::Menu::createInstance(heap);
	menu->init(heap);

	cly::Server* server = cly::Server::createInstance(heap);
	server->init(heap, "192.168.2.62");
	server->connect();

	cly::tas::System* system = cly::tas::System::createInstance(heap);
	system->init(heap);

	cly::endInit();

	gameSystemInit.orig(gameSystem);
});

HkTrampoline<void, GameSystem*> gameSystemDraw = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
	if (cly::tas::Pauser::instance()->isSequenceActive()) {
		gameSystemDraw.orig(gameSystem);
	}

	cly::Menu::instance()->draw();
});

HkTrampoline<void, GameSystem*> gameSystemUpdate = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
	if (cly::tas::Pauser::instance()->isSequenceActive()) {
		gameSystemUpdate.orig(gameSystem);
	}

	// cly::tas::Pauser::instance()->update();
	// cly::Menu::instance()->handleInput(al::getMainControllerPort());
});

HkTrampoline<void, al::Scene*> sceneUpdate = hk::hook::trampoline([](al::Scene* scene) -> void {
	sceneUpdate.orig(scene);
	if (cly::tas::System::isReplaying()) cly::tas::System::getNextFrame();
});

HkTrampoline<void, al::NpadController*> inputHook = hk::hook::trampoline([](al::NpadController* controller) -> void {
	inputHook.orig(controller);

	if (!cly::gIsInitialized) return;

	// cly::Menu::log(
	// 	"%d %d %d %d %08x %d", controller->mIsConnected ? 1 : 0, controller->mControllerMode, controller->mNpadId,
	// controller->mNpadStyleTag, 	controller->mPadHold, controller->mSamplingNumber
	// );

	if (controller->mControllerMode == -1 || controller->mControllerMode == 0) {
		cly::tas::Pauser::instance()->update();
		cly::Menu::instance()->handleInput(controller->mPadHold);
	}

	// if (cly::Menu::isActive()) {
	// 	controller->mPadHold.makeAllZero();
	// 	controller->mLeftStick.set(sead::Vector2f::zero);
	// 	controller->mRightStick.set(sead::Vector2f::zero);
	// }

	if (!cly::tas::System::isReplaying()) return;

	if (controller->mControllerMode == -1 || controller->mControllerMode == 0) {
		// player 1
		cly::tas::InputFrame inputFrame;
		cly::tas::System::tryReadCurFrame(&inputFrame);
		controller->mPadHold = cly::tas::convertButtonsSTASToSead(inputFrame.padHold);
		controller->mLeftStick.set(inputFrame.leftStick);
		controller->mRightStick.set(inputFrame.rightStick);
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
	gameSystemDraw.installAtSym<"_ZN10GameSystem8drawMainEv">();
	gameSystemUpdate.installAtSym<"_ZN10GameSystem8movementEv">();
	sceneUpdate.installAtSym<"_ZN2al5Scene8movementEv">();
	inputHook.installAtSym<"_ZN2al14NpadController9calcImpl_Ev">();
	fileDeviceMgrHook.installAtSym<"_ZN4sead13FileDeviceMgrC1Ev">();
}
