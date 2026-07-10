#include "hk/hook/Trampoline.h"
#include "hooks.h"
#include "smo/NintendoSDK/nn/fs/fs_mount.h"
#include "smo/sead/filedevice/seadFileDeviceMgr.h"

namespace cly::hooks {
void Hooks::setupFs() {
	HkTrampoline fileDeviceMgrHook = [](TrampolineStatic(), sead::FileDeviceMgr* fileDeviceMgr) -> void {
		orig(fileDeviceMgr);

		fileDeviceMgr->mMountedSd = nn::fs::MountSdCardForDebug("sd") == 0;
	};
	fileDeviceMgrHook.installAtSym<"_ZN4sead13FileDeviceMgrC1Ev">();
}
} // namespace cly::hooks
