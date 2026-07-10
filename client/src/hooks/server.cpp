#include "server.h"
#include "hk/hook/Trampoline.h"
#include "hooks.h"
#include "menu.h"
#include "smo/al/Library/LiveActor/ActorPoseUtil.h"
#include "smo/game/Scene/StageScene.h"
#include "smo/game/Sequence/HakoniwaSequence.h"
#include "smo/game/System/GameDataFunction.h"
#include "smo/game/System/GameSystem.h"

namespace cly::hooks {
void installDiscovery() {
	static HkTrampoline hook = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
		static u8 discoveryTimer = 30;

		if (--discoveryTimer == 0) {
			discoveryTimer = 30;
			cly::Server::instance()->sendUDPDiscoveryBroadcast();
		}
		orig(gameSystem);
	};
	hook.installAtSym<"_ZN10GameSystem8movementEv">();
}

void installHandleChangeStage() {
	static HkTrampoline hook = [](TrampolineStatic(), HakoniwaSequence* sequence) -> void {
		auto server = cly::Server::instance();

		auto has = true;
		if (server->changeStageInfo.mHasChangeStageInfo.compare_exchange_strong(has, false, std::memory_order_seq_cst, std::memory_order_relaxed)) {
			GameDataHolder* gameDataHolder = sequence->mGameDataHolderAccessor;
			if (server->changeStageInfo.mSimpleReload) {
				cly::Server::log("NOT restarting stage (todo: reloads)");
			} else {
				cly::Menu::log("changing to stage %s", server->changeStageInfo.mStageName.data());
				ChangeStageInfo info(
					gameDataHolder, server->changeStageInfo.mEntranceName.data(), server->changeStageInfo.mStageName.data(), server->changeStageInfo.mIsReturn,
					server->changeStageInfo.mScenario, server->changeStageInfo.mSubScenario
				);

				GameDataFunction::tryChangeNextStage(gameDataHolder, &info);
				cly::Server::log("changing to stage %s", server->changeStageInfo.mStageName.data());
			}
			server->changeStageInfo.mHasChangeStageInfo = false;
		}
		orig(sequence);
	};

	hook.installAtSym<"_ZN16HakoniwaSequence12exePlayStageEv">();
}

void installReportStageName() {
	HkTrampoline sceneInit = [](TrampolineStatic(), al::Scene* scene, const char* stageName, s32 scenario) -> void {
		cly::Server::reportStageName(stageName, scenario);
		orig(scene, stageName, scenario);
	};
}

void installReportPlayerPosition() {
	HkTrampoline sceneUpdate = [](TrampolineStatic(), al::Scene* scene) -> void {
		orig(scene);
		if (auto player = rs::getPlayerActor(scene)) {
			cly::Server::reportPlayerPosition(al::getTrans(player));
		}
	};
}

void Hooks::setupServer() {
	installDiscovery();
	installHandleChangeStage();
	installReportStageName();
	installReportPlayerPosition()
}
} // namespace cly::hooks
