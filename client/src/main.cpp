#include "hk/hook/a64/Assembler.h"
#include "hk/ro/RoUtil.h"
#include "hooks/hooks.h"
#include "menu.h"
#include "server.h"
#include "smo/al/Library/Base/StringUtil.h"
#include "smo/game/System/GameDataFunction.h"
#include "smo/game/System/GameDataHolder.h"
#include "smo/game/System/GameDataHolderAccessor.h"
#include "smo/sead/controller/seadControllerDefine.h"
#include "tas.h"

#include <atomic>
#include <hk/Result.h>
#include <hk/diag/diag.h>
#include <hk/gfx/DebugRenderer.h>
#include <hk/hook/Trampoline.h>
#include <hk/util/Context.h>

#include <agl/common/aglDrawContext.h>
#include <nn/fs.h>
#include <nn/hid.h>
#include <sead/controller/seadAccelerometerAddon.h>
#include <sead/controller/seadControllerAddon.h>
#include <sead/controller/seadControllerMgr.h>
#include <sead/filedevice/seadFileDeviceMgr.h>
#include <sead/heap/seadHeap.h>
#include <sead/heap/seadHeapMgr.h>
#include <sead/math/seadVector.h>

#include "al/Library/Controller/NpadController.h"
#include "al/Library/LiveActor/ActorPoseUtil.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/Nerve/Nerve.h"
#include "al/Library/Nerve/NerveUtil.h"
#include "al/Library/Scene/Scene.h"
#include "game/Scene/StageScene.h"
#include "game/Sequence/HakoniwaSequence.h"
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



extern "C" void hkMain() {
	hook::a64::assemble<"mov x0, #1\nsvc #0x28">().installAtOffset(ro::getRtldModule(), 0);
	hk::gfx::DebugRenderer::instance()->installHooks();

	cly::hooks::Hooks::setup();
}
