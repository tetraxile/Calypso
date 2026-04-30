#include "tas.h"
#include "menu.h"
#include "server.h"

#include <hk/diag/diag.h>

#include <nn/fs.h>
#include <sead/controller/seadController.h>
#include <sead/heap/seadHeap.h>

namespace cly::tas {

SEAD_SINGLETON_DISPOSER_IMPL(System);
SEAD_SINGLETON_DISPOSER_IMPL(Pauser);

void System::init(sead::Heap* heap) {
	mHeap = heap;
}

void System::getNextFrame() {
	System* self = instance();
	Pauser::instance()->setBlocked(false);
	if (!self->isReplaying()) {
		Menu::log("ERROR: no script loaded");
		return;
	}

	if (self->mFrameIdx >= self->mScriptInfo.frameCount) {
		if (self->mFrameIdx == self->mScriptInfo.frameCount) {
			self->mFrameIdx++;
			Menu::log("script ended?");
			Server::reportScriptCompleted();
			self->stopReplay();
		}
		return;
	}

	if (!self->mHasCurFrame || self->mCurFrame.frameIndex <= self->mFrameIdx) {
		auto result = Server::instance()->mFrameBuffer.popBack();
		if (!result.hasValue()) {
			Pauser::instance()->setBlocked(true);
			return;
		}

		self->mCurFrame = HK_UNWRAP(result);
		self->mHasCurFrame = true;
	}

	Menu::log("%03d: %016lx %016lx", self->mFrameIdx, self->mCurFrame.player1.buttons);
	self->mFrameIdx++;
}

hk::ValueOrResult<Server::FramePacket> System::tryReadCurFrame() {
	System* self = instance();

	bool isReplay = self->isReplaying();
	if (isReplay) {
		self->mHasCurFrame = false;
		return self->mCurFrame;
	}

	return hk::ResultFailed();
}

void System::startReplay() {
	System* self = instance();
	if (self->mIsReplaying) return;

	self->mIsReplaying = true;
	Menu::log("started replaying");
}

void System::stopReplay() {
	System* self = instance();
	if (self->mIsReplaying) {
		self->mIsReplaying = false;
		self->mFrameIdx = 0;
		self->mHasCurFrame = false;
		Menu::log("stopped replaying");
	}
}

sead::BitFlag32 convertButtonsSTASToSead(sead::BitFlag64 stasPad) {
	sead::BitFlag32 mask = 0;
	for (s32 i = 0; i < 64; i++) {
		if (stasPad.isOnBit(i)) {
			if (i == cSTAS_A)
				mask.setBit(sead::Controller::cPadIdx_A);
			else if (i == cSTAS_B)
				mask.setBit(sead::Controller::cPadIdx_B);
			else if (i == cSTAS_X)
				mask.setBit(sead::Controller::cPadIdx_X);
			else if (i == cSTAS_Y)
				mask.setBit(sead::Controller::cPadIdx_Y);
			else if (i == cSTAS_LeftStick)
				mask.setBit(sead::Controller::cPadIdx_1);
			else if (i == cSTAS_RightStick)
				mask.setBit(sead::Controller::cPadIdx_2);
			else if (i == cSTAS_L)
				mask.setBit(sead::Controller::cPadIdx_L);
			else if (i == cSTAS_R)
				mask.setBit(sead::Controller::cPadIdx_R);
			else if (i == cSTAS_ZL)
				mask.setBit(sead::Controller::cPadIdx_ZL);
			else if (i == cSTAS_ZR)
				mask.setBit(sead::Controller::cPadIdx_ZR);
			else if (i == cSTAS_Plus) {
				mask.setBit(sead::Controller::cPadIdx_Plus);
				mask.setBit(sead::Controller::cPadIdx_Start);
			} else if (i == cSTAS_Minus)
				mask.setBit(sead::Controller::cPadIdx_Minus);
			else if (i == cSTAS_DLeft)
				mask.setBit(sead::Controller::cPadIdx_Left);
			else if (i == cSTAS_DUp)
				mask.setBit(sead::Controller::cPadIdx_Up);
			else if (i == cSTAS_DRight)
				mask.setBit(sead::Controller::cPadIdx_Right);
			else if (i == cSTAS_DDown)
				mask.setBit(sead::Controller::cPadIdx_Down);
			// else if (i == cSTAS_LeftStickLeft)
			// 	mask.setBit(sead::Controller::cPadIdx_LeftStickLeft);
			// else if (i == cSTAS_LeftStickUp)
			// 	mask.setBit(sead::Controller::cPadIdx_LeftStickUp);
			// else if (i == cSTAS_LeftStickRight)
			// 	mask.setBit(sead::Controller::cPadIdx_LeftStickRight);
			// else if (i == cSTAS_LeftStickDown)
			// 	mask.setBit(sead::Controller::cPadIdx_LeftStickDown);
			// else if (i == cSTAS_RightStickLeft)
			// 	mask.setBit(sead::Controller::cPadIdx_RightStickLeft);
			// else if (i == cSTAS_RightStickUp)
			// 	mask.setBit(sead::Controller::cPadIdx_RightStickUp);
			// else if (i == cSTAS_RightStickRight)
			// 	mask.setBit(sead::Controller::cPadIdx_RightStickRight);
			// else if (i == cSTAS_RightStickDown)
			// 	mask.setBit(sead::Controller::cPadIdx_RightStickDown);
		}
	}

	return mask;
}

} // namespace cly::tas
