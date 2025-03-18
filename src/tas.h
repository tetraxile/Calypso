#pragma once

#include <nn/fs.h>
#include <sead/heap/seadDisposer.h>
#include <sead/math/seadVector.h>
#include <sead/prim/seadSafeString.h>

namespace cly::tas {

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

struct InputFrame {
	u64 padTrig = 0;
	u64 padHold = 0;
	sead::Vector2f leftStick = sead::Vector2f::zero;
	sead::Vector2f rightStick = sead::Vector2f::zero;
};

enum class CommandType : u16 {
	FRAME = 0,
	CONTROLLER = 1,
	MOTION = 2,
	AMIIBO = 3,
	TOUCH = 4,

	INVALID = 0xffff
};

class System {
	SEAD_SINGLETON_DISPOSER(System);

private:
	struct ScriptInfo {
		ScriptInfo() { clear(); }

		void clear();

		bool isLoaded = false;
		u8 playerCount;
		u32 commandCount;
		u8 controllerTypes[8];
	};

	template <typename T>
	T read() {
		T value = *(T*)&mScriptData[mCursor];
		mCursor += sizeof(T);
		return value;
	}

	s32 readScriptHeader();
	bool tryReadCommand(CommandType* cmdType);

	sead::Heap* mHeap = nullptr;
	ScriptInfo mScriptInfo;
	s64 mScriptLength = 0;
	u8* mScriptData = nullptr;
	u64 mCursor = 0;
	u32 mCommandIdx = 0;
	u32 mFrameIdx = 0;
	u32 mNextFrameIdx = 0;
	bool mIsReplaying = false;
	InputFrame mPrevFrame;
	InputFrame mCurFrame;

public:
	System() = default;
	void init(sead::Heap* heap);
	static void loadScript(const sead::SafeString& filename);
	static void unloadScript();
	static void startReplay();
	static void stopReplay();
	static bool isReplaying();
	static u32 getFrameCount(); // is this possible to implement with stas format?
	static void getNextFrame();
};

} // namespace cly::tas
