#include "hk/hook/Trampoline.h"
#include "hooks/hooks.h"
#include "server.h"
#include "smo/NintendoSDK/nn/hid.h"
#include "smo/al/Library/Base/StringUtil.h"
#include "smo/al/Library/Nerve/Nerve.h"
#include "smo/al/Library/Nerve/NerveUtil.h"
#include "smo/al/Library/Scene/Scene.h"
#include "smo/game/System/GameDataFile.h"
#include "smo/game/System/GameDataHolderAccessor.h"
#include "util.h"

namespace cly::hooks {
void installStickReporting() {
	static HkTrampoline hook = [](TrampolineStatic(), nn::hid::NpadJoyDualState* states, s32 count, const u32& port) -> void {
		orig(states, count, port);
		if (count >= 1) {
			cly::Server::reportInput(states[0]);
		}
	};
	hook.installAtSym<"_ZN2nn3hid13GetNpadStatesEPNS0_16NpadJoyDualStateEiRKj">();
}

void installShowUi() {
	HkTrampoline drawKit = [](TrampolineStatic(), al::Scene* scene, const char* executorList) -> void {
		bool showUi = cly::Server::instance()->tools.showUi;
		if (!showUi && al::isEndWithString(util::getTypeName(al::getCurrentNerve(scene)), "StageSceneNrvPlayE") && al::isStartWithString(executorList, "２Ｄ"))
			return;
		orig(scene, executorList);
	};
	HkTrampoline drawKitList = [](TrampolineStatic(), al::Scene* scene, const char* executorList, const char* otherExecutorList) -> void {
		bool showUi = cly::Server::instance()->tools.showUi;
		if (!showUi && al::isEndWithString(util::getTypeName(al::getCurrentNerve(scene)), "StageSceneNrvPlayE") &&
		    (al::isStartWithString(executorList, "２Ｄ") || al::isStartWithString(otherExecutorList, "２Ｄ")))
			return;
		orig(scene, executorList, otherExecutorList);
	};

	drawKit.installAtSym<"_ZN2al7drawKitEPKNS_5SceneEPKc">();
	drawKitList.installAtSym<"_ZN2al11drawKitListEPKNS_5SceneEPKcS4_">();
}

void installUncollectedMoons() {
	HkTrampoline isGotShineHook = [](TrampolineStatic(), GameDataHolderAccessor accessor, ShineInfo* info) -> bool {
		if (!cly::Server::instance()->tools.alwaysUncollectedMoons) return orig(accessor, info);
		return false;
	};
	isGotShineHook.installAtSym<"_ZN16GameDataFunction10isGotShineE22GameDataHolderAccessorPK9ShineInfo">();
}

void Hooks::setupBasic() {
	installStickReporting();
	installShowUi();
	installUncollectedMoons();
}
} // namespace cly::hooks
