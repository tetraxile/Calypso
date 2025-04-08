#pragma once

#include <controller/seadController.h>
#include <nn/hid.h>

namespace al {
class NpadController : public sead::Controller {
	SEAD_RTTI_OVERRIDE(NpadController, sead::Controller);

public:
	NpadController(sead::ControllerMgr* controllerMgr);
	~NpadController() override = default;
	bool isConnected() const override;
	bool isValidNpadId() const;
	s32 getNpadId() const;
	void setAnyControllerMode();
	void setIndexControllerMode(s32 idx);
	const void* getVibrationDeviceHandle(s32 idx) const;

	void calcImpl_() override;

	s32 mControllerMode = -1;
	s32 mNpadId = -1;
	nn::hid::NpadStyleTag mNpadStyleTag = nn::hid::NpadStyleTag::NpadStyleInvalid;
	s32 mSixAxisDeviceNum = 0;
	s32 mVibrationDeviceNum = 0;
	bool mIsConnected = false;
	u64 mSamplingNumber = 0;
};
} // namespace al
