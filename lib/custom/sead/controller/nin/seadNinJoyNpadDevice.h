#pragma once

#include <nn/hid.h>

#include "controller/seadControlDevice.h"
#include "controller/seadController.h"
#include "heap/seadHeap.h"

namespace sead {
// stubbed and only added methods for seadControllerMgr
class NinJoyNpadDevice : public ControlDevice {
public:
	class VibrationThread {
		u8 padding[0x298];
	};

	NinJoyNpadDevice(ControllerMgr* mgr, Heap* heap);

	void calc() override;

private:
	u32 mNpadIdUpdateNum;
	nn::hid::NpadJoyHoldType mNpadJoyHoldType;
	nn::hid::NpadStyleTag mNpadStyleTags[9];
	nn::hid::NpadHandheldState mNpadStates[9];
	VibrationThread mVibrationThread;
};

} // namespace sead
