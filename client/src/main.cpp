#include "main.h"
#include "menu.h"
#include "server.h"
#include "tas.h"

#include <hk/Result.h>
#include <hk/diag/diag.h>
#include <hk/gfx/DebugRenderer.h>
#include <hk/hook/Trampoline.h>
#include <hk/hook/a64/Assembler.h>
#include <hk/ro/RoUtil.h>
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

#include "Library/Memory/HeapUtil.h"

using namespace hk;

namespace cly {
bool gIsInitialized = false;

sead::Heap* initializeHeap() {
	return sead::ExpHeap::create(1_MB, "CalypsoHeap", al::getStationedHeap(), 8, sead::Heap::cHeapDirection_Forward, false);
}

void initSystem() {
	sead::Heap* heap = initializeHeap();

	tas::Pauser* pauser = tas::Pauser::createInstance(heap);

	Menu* menu = Menu::createInstance(heap);
	menu->init(heap);

	Server* server = Server::createInstance(heap);
	server->init(heap);

	tas::System* system = tas::System::createInstance(heap);
	system->init(heap);

	gIsInitialized = true;
}
} // namespace cly

extern "C" void hkMain() {
	hook::a64::assemble<"mov x0, #1\nsvc #0x28">().installAtOffset(ro::getRtldModule(), 0);
	hk::gfx::DebugRenderer::instance()->installHooks();
	cly::setupHooks();
}
