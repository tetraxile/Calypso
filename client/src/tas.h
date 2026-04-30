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
	bool mIsReplaying = false;
	bool mHasCurFrame = true;
	Server::FramePacket mCurFrame;

public:
	System() = default;
	void init(sead::Heap* heap);
	static void startReplay();
	static void stopReplay();
	static bool isReplaying() { return instance()->mIsReplaying; }
	static u32 getFrameIndex() { return instance()->mFrameIdx; };
	static u32 getFrameCount() { return instance()->mScriptInfo.frameCount; };
	static void getNextFrame();
	static hk::ValueOrResult<Server::FramePacket> tryReadCurFrame();

	static void setScriptInfo(Server::ScriptInfoPacket scriptInfo) { instance()->mScriptInfo = scriptInfo; }
};

class Pauser {
	SEAD_SINGLETON_DISPOSER(Pauser);

private:
	bool mIsPaused = false;
	bool mIsBlocked = false;
	std::atomic<s32> mFrameAdvance = 0;

	bool isPaused() const { return mIsPaused || mIsBlocked; }

public:
	Pauser() = default;

	void setBlocked(bool isBlocked) { mIsBlocked = isBlocked; }

	void pause() { mIsPaused = true; }

	void play() { mIsPaused = false; }

	void togglePause() { mIsPaused ^= 1; }

	void advanceFrame() { mFrameAdvance++; }

	bool isSequenceActive() const { return !isPaused() || (mFrameAdvance != 0); }

	void update() {
		if (mFrameAdvance > 0 && !mIsBlocked) {
			mFrameAdvance--;
		}
	}
};

sead::BitFlag32 convertButtonsSTASToSead(sead::BitFlag64 stasPad);

} // namespace cly::tas
