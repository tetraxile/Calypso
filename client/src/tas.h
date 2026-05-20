#pragma once

#include <atomic>
#include <nn/fs.h>
#include <sead/heap/seadDisposer.h>
#include <sead/math/seadVector.h>
#include <sead/prim/seadBitFlag.h>
#include <sead/prim/seadSafeString.h>
#include "server.h"

namespace cly::tas {

enum Button : s32 {
	cSTAS_A = 0,
	cSTAS_B,
	cSTAS_X,
	cSTAS_Y,
	cSTAS_LeftStick,
	cSTAS_RightStick,
	cSTAS_L,
	cSTAS_R,
	cSTAS_ZL,
	cSTAS_ZR,
	cSTAS_Plus,
	cSTAS_Minus,
	cSTAS_DLeft,
	cSTAS_DUp,
	cSTAS_DRight,
	cSTAS_DDown,
};

#pragma pack(push, 4)

struct FileHeader {
	char magic[4];
	u16 formatVersion;
	u16 addonsVersion;
	u16 editorVersion;
	u64 gameTitleId;
};

struct ScriptHeader {
	s32 commandCount;
	s32 secondsEditing;
	u8 playerCount;
};

#pragma pack(pop)

class System {
	SEAD_SINGLETON_DISPOSER(System);

private:
	sead::Heap* mHeap = nullptr;
	Server::ScriptInfoPacket mScriptInfo;
	u32 mFrameIdx = 0;
	u32 mNextFrameIdx = 0;
	bool mIsReplaying = false;
	bool mHasCurFrame = false;
	Server::FramePacket mCurFrame;

public:
	System() = default;
	void init(sead::Heap* heap);
	static void startReplay();
	static void stopReplay();

	static bool isReplaying() { return instance()->mIsReplaying; }

	static bool isApplyingInput();

	static u32 getFrameIndex() { return instance()->mFrameIdx; };

	static u32 getServerIndex() { return instance()->mCurFrame.serverIndex; };

	static u32 getFrameCount() { return instance()->mScriptInfo.frameCount; };

	static void checkForNextFrame();
	static void getNextFrame();
	static hk::ValueOrResult<Server::FramePacket> tryReadCurFrame();

	static void setScriptInfo(Server::ScriptInfoPacket scriptInfo) { instance()->mScriptInfo = scriptInfo; }

	static bool isDualJoycons(s32 index) { return instance()->mScriptInfo.controllerTypes[index] == 3; }
};

class Pauser {
	SEAD_SINGLETON_DISPOSER(Pauser);

private:
	std::atomic_bool mIsPaused = false;
	bool mIsBlocked = false;
	std::atomic_bool mWaitingOnLoad = false;
	std::atomic<s32> mFrameAdvance = 0;
	std::atomic<s32> mLoadDelay = 0;

	bool isPaused() const { return mIsPaused || mIsBlocked; }

public:
	Pauser() = default;

	bool isBlocked() const { return mIsBlocked; }

	bool isWaitingOnLoad() const { return mWaitingOnLoad; }

	bool isManuallyPaused() const { return mIsPaused; }

	void setWaitingOnLoad(bool waiting) { mWaitingOnLoad = waiting; }

	void setBlocked(bool isBlocked) { mIsBlocked = isBlocked; }

	void pause() { mIsPaused = true; }

	void play() { mIsPaused = false; }

	void togglePause() { mIsPaused = !mIsPaused; }

	void advanceFrame() { mFrameAdvance++; }

	bool isSequenceActive() const { return !isPaused() || isWaitingOnLoad() || (mFrameAdvance != 0); }

	void update() {
		if (mFrameAdvance > 0 && !mIsBlocked) {
			mFrameAdvance--;
		}
	}
};

sead::BitFlag32 convertButtonsSTASToSead(sead::BitFlag64 stasPad);

} // namespace cly::tas
