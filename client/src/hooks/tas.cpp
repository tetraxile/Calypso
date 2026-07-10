#include "tas.h"
#include "Library/Controller/NpadController.h"
#include "hk/hook/Trampoline.h"
#include "hooks/hooks.h"
#include "main.h"
#include "menu.h"
#include "smo/al/Library/Scene/Scene.h"
#include "smo/game/System/GameSystem.h"
#include "smo/sead/controller/seadAccelerometerAddon.h"

namespace cly::hooks {
void Hooks::setupTas() {
	HkTrampoline gameSystemDraw = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
		if (cly::tas::Pauser::instance()->isSequenceActive()) {
			orig(gameSystem);
		}
		cly::tas::Pauser::instance()->update();

		cly::Menu::instance()->draw();
	};

	HkTrampoline gameSystemUpdate = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
		if (cly::tas::Pauser::instance()->isSequenceActive()) {
			orig(gameSystem);
		}

		cly::tas::System::checkForNextFrame();
	};

	HkTrampoline sceneInit = [](TrampolineStatic(), al::Scene* scene, const char* stageName, s32 scenario) -> void {
		cly::Server::reportStageName(stageName, scenario);
		cly::tas::Pauser::instance()->setWaitingOnLoad(false);
		orig(scene, stageName, scenario);
	};

	HkTrampoline sceneUpdate = [](TrampolineStatic(), al::Scene* scene) -> void {
		orig(scene);
		if (cly::tas::System::isReplaying()) cly::tas::System::getNextFrame();
	};
	HkTrampoline inputHook = [](TrampolineStatic(), al::NpadController* controller) -> void {
		orig(controller);

		if (!cly::gIsInitialized) return;

		// cly::Menu::log(
		// 	"%d %d %d %d %08x %d", controller->mIsConnected ? 1 : 0, controller->mControllerMode, controller->mNpadId,
		// controller->mNpadStyleTag, 	controller->mPadHold, controller->mSamplingNumber
		// );

		if (controller->mControllerMode == -1 || controller->mControllerMode == 0) {
			cly::Menu::instance()->handleInput(controller->mPadHold);
		}

		if (!tas::System::isApplyingInput()) return;

		// if (cly::Menu::isActive()) {
		// 	controller->mPadHold.makeAllZero();
		// 	controller->mLeftStick.set(sead::Vector2f::zero);
		// 	controller->mRightStick.set(sead::Vector2f::zero);
		// }

		if (controller->mControllerMode == -1 || controller->mControllerMode == 0) {
			// player 1
			auto frame = cly::tas::System::tryReadCurFrame();
			controller->mPadHold = cly::tas::convertButtonsSTASToSead(frame.player1.buttons);
			controller->mLeftStick.set({ f32(frame.player1.leftStick.x) / 32767.f, f32(frame.player1.leftStick.y) / 32767.f });
			controller->mRightStick.set({ f32(frame.player1.rightStick.x) / 32767.f, f32(frame.player1.rightStick.y) / 32767.f });
			auto applyAccel = [&](s32 index, const sead::Vector3f& accel) {
				auto addon = (sead::AccelerometerAddon*)controller->getAddonByOrder(sead::ControllerDefine::cAddon_Accelerometer, index);
				addon->mAcceleration.set(accel);
			};
			applyAccel(0, frame.player1.accelLeft);
			if (cly::tas::System::isDualJoycons(0)) applyAccel(1, frame.player1.accelRight);
		} else if (controller->mControllerMode == 1) {
			// player 2
		}
	};

	gameSystemDraw.installAtSym<"_ZN10GameSystem8drawMainEv">();
	gameSystemUpdate.installAtSym<"_ZN10GameSystem8movementEv">();
	sceneInit.installAtSym<"_ZN2al5Scene24initAndLoadStageResourceEPKci">();
	sceneUpdate.installAtSym<"_ZN2al5Scene8movementEv">();
	inputHook.installAtSym<"_ZN2al14NpadController9calcImpl_Ev">();
}
} // namespace cly::hooks
