#pragma once

#include <controller/seadController.h>

namespace al {
class NpadController : public sead::Controller {
    SEAD_RTTI_OVERRIDE(NpadController, sead::Controller);

public:
    NpadController(sead::ControllerMgr* controllerMgr);
    bool isConnected() const override;
    bool isValidNpadId() const;
    s32 getNpadId() const;
    void setAnyControllerMode();
    void setIndexControllerMode(s32 idx);
    void* getVibrationDeviceHandle(s32 idx) const;
    void calcImpl_() override;

private:
    s32 mControllerMode = -1;
    s32 mNpadId = -1;
    s32 _180 = 5;
    s32 mDeviceNum = 0;
    s32 _188 = 0;
    bool _18c = false;
    u64 mSamplingNumber = 0;
};
}  // namespace al
