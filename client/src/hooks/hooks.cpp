#include "hooks/hooks.h"
#include "hk/hook/Trampoline.h"
#include "menu.h"
#include "smo/game/System/GameSystem.h"
#include "smo/sead/heap/seadHeap.h"
#include "tas.h"

namespace cly {
sead::Heap* initializeHeap();
void endInit();

namespace hooks {
void Hooks::setupInit() {
	HkTrampoline gameSystemInit = [](TrampolineStatic(), GameSystem* gameSystem) -> void {
		sead::Heap* heap = initializeHeap();

		cly::tas::Pauser* pauser = cly::tas::Pauser::createInstance(heap);

		cly::Menu* menu = cly::Menu::createInstance(heap);
		menu->init(heap);

		cly::Server* server = cly::Server::createInstance(heap);
		server->init(heap);

		cly::tas::System* system = cly::tas::System::createInstance(heap);
		system->init(heap);

		endInit();

		orig(gameSystem);
	};
	gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();
}
} // namespace hooks
} // namespace cly
