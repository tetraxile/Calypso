#pragma once

#include <nn/fs.h>
#include <sead/heap/seadDisposer.h>
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

	s32 readScriptHeader();

	sead::Heap* mHeap;
	ScriptInfo mScriptInfo;
	s64 mScriptLength = 0;
	u8* mScriptData = nullptr;

public:
	System() = default;
	void init(sead::Heap* heap);
	static void loadScript(const sead::SafeString& filename);
	static void unloadScript();
};

} // namespace cly::tas
