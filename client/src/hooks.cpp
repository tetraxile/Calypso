#include <hk/hook/Trampoline.h>

#include <sead/filedevice/seadFileDeviceMgr.h>

#include "Library/Base/StringUtil.h"
#include "Library/LiveActor/ActorPoseUtil.h"
#include "Library/Nerve/Nerve.h"
#include "Library/Nerve/NerveUtil.h"
#include "Library/Scene/Scene.h"

#include "Scene/StageScene.h"
#include "Sequence/HakoniwaSequence.h"
#include "System/GameDataFunction.h"
#include "System/GameSystem.h"

#include "main.h"
#include "menu.h"
#include "tas.h"

namespace cly {
void setupHooks() {
	HkTrampoline gameSystemInit = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
		initSystem();
		orig(gameSystem);
	};

	HkTrampoline gameSystemDraw = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
		if (tas::Pauser::instance()->isSequenceActive()) orig(gameSystem);
		tas::Pauser::instance()->update();
		Menu::instance()->draw();
	};

	HkTrampoline gameSystemUpdate = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
		if (tas::Pauser::instance()->isSequenceActive()) orig(gameSystem);

		static u8 discoveryTimer = 30;
		if (--discoveryTimer == 0) {
			discoveryTimer = 30;
			Server::instance()->sendUDPDiscoveryBroadcast();
		}

		tas::System::checkForNextFrame();
	};

	HkTrampoline sceneInit = [](TrampolineStatic(), al::Scene* scene, const char* stageName, s32 scenarioNo) -> void {
		Server::reportStageName(stageName, scenarioNo);
		tas::Pauser::instance()->setWaitingOnLoad(false);
		orig(scene, stageName, scenarioNo);
	};

	HkTrampoline sceneMovement = [](TrampolineStatic(), al::Scene* scene) -> void {
		orig(scene);
		if (al::LiveActor* player = rs::getPlayerActor(scene)) Server::reportPlayerPosition(al::getTrans(player));
		if (tas::System::isReplaying()) tas::System::getNextFrame();
	};

	HkTrampoline npadControllerCalc = [](TrampolineStatic(), al::NpadController* controller) -> void {
		orig(controller);
		tas::System::processInputs(controller);
	};

	HkTrampoline fileDeviceMgrCtor = [](TrampolineStatic(), sead::FileDeviceMgr* fileDeviceMgr) -> void {
		orig(fileDeviceMgr);
		fileDeviceMgr->mMountedSd = nn::fs::MountSdCardForDebug("sd") == 0;
	};

	HkTrampoline sequenceNrvPlayStage = [](TrampolineStatic(), HakoniwaSequence* sequence) -> void {
		Server::handleStageChange(sequence);
		orig(sequence);
	};

	static HkTrampoline getNpadStates = [](TrampolineStatic(), nn::hid::NpadJoyDualState* states, s32 count, const u32& port) -> void {
		orig(states, count, port);
		if (count >= 1) Server::reportInput(states[0]);
	};

	HkTrampoline drawKit = [](TrampolineStatic(), al::Scene* scene, const char* executorList) -> void {
		bool showUi = Server::instance()->tools.showUi;
		bool isSceneNrvPlay = al::isEndWithString(util::getTypeName(al::getCurrentNerve(scene)), "StageSceneNrvPlayE");
		bool isList2D = al::isStartWithString(executorList, "２Ｄ");
		if (!showUi && isSceneNrvPlay && isList2D) return;
		orig(scene, executorList);
	};

	HkTrampoline drawKitList = [](TrampolineStatic(), al::Scene* scene, const char* executorList, const char* otherExecutorList) -> void {
		bool showUi = Server::instance()->tools.showUi;
		bool isSceneNrvPlay = al::isEndWithString(util::getTypeName(al::getCurrentNerve(scene)), "StageSceneNrvPlayE");
		bool isList2D = al::isStartWithString(executorList, "２Ｄ") || al::isStartWithString(otherExecutorList, "２Ｄ");
		if (!showUi && isSceneNrvPlay && isList2D) return;
		orig(scene, executorList, otherExecutorList);
	};

	HkTrampoline isGotShine = [](TrampolineStatic(), GameDataHolderAccessor accessor, ShineInfo* info) -> bool {
		if (!Server::instance()->tools.alwaysUncollectedMoons) return orig(accessor, info);
		return false;
	};

	gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
	gameSystemDraw.installAtSym<"_ZN10GameSystem8drawMainEv">();
	gameSystemUpdate.installAtSym<"_ZN10GameSystem8movementEv">();
	sceneInit.installAtSym<"_ZN2al5Scene24initAndLoadStageResourceEPKci">();
	sceneMovement.installAtSym<"_ZN2al5Scene8movementEv">();
	drawKit.installAtSym<"_ZN2al7drawKitEPKNS_5SceneEPKc">();
	drawKitList.installAtSym<"_ZN2al11drawKitListEPKNS_5SceneEPKcS4_">();
	npadControllerCalc.installAtSym<"_ZN2al14NpadController9calcImpl_Ev">();
	getNpadStates.installAtSym<"_ZN2nn3hid13GetNpadStatesEPNS0_16NpadJoyDualStateEiRKj">();
	fileDeviceMgrCtor.installAtSym<"_ZN4sead13FileDeviceMgrC1Ev">();
	sequenceNrvPlayStage.installAtSym<"_ZN16HakoniwaSequence12exePlayStageEv">();
	isGotShine.installAtSym<"_ZN16GameDataFunction10isGotShineE22GameDataHolderAccessorPK9ShineInfo">();
}
} // namespace cly
