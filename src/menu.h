#pragma once

#include <hk/gfx/DebugRenderer.h>

#include <sead/container/seadPtrArray.h>
#include <sead/gfx/seadPrimitiveRenderer.h>
#include <sead/gfx/seadTextWriter.h>
#include <sead/heap/seadDisposer.h>
#include <sead/prim/seadSafeString.h>
#include "menuitem.h"

namespace tas {

class MenuItem;

class Menu {
	SEAD_SINGLETON_DISPOSER(Menu);

private:
	constexpr static s32 cMenuItemNumMax = 128;
	constexpr static s32 cLogEntryNumMax = 10;

	struct LogEntry {
		constexpr static s32 cFadeLength = 30;
		constexpr static s32 cStartFade = 270;
		constexpr static s32 cEndFade = cStartFade + cFadeLength;

		s32 age = 0;
		sead::FixedSafeString<128> text = sead::SafeString::cEmptyString;
	};

	bool mIsActive = true;
	sead::Heap* mHeap = nullptr;
	hk::gfx::DebugRenderer* mRenderer = nullptr;

	const hk::util::Vector2i mScreenResolution = { 1280, 720 };
	hk::util::Vector2i mCellResolution = { 12, 36 };
	hk::util::Vector2f mCellDimension = { (f32)mScreenResolution.x / mCellResolution.x, (f32)mScreenResolution.y / mCellResolution.y };
	f32 mFontHeight = mCellDimension.y;
	f32 mShadowOffset = 2.0f;

	sead::PtrArray<MenuItem> mItems;
	MenuItem* mSelectedItem = nullptr;

	sead::FixedPtrArray<LogEntry, cLogEntryNumMax> mLog;

	hk::util::Vector2f cellPosToAbsolute(const hk::util::Vector2i& cellPos) { return { cellPos.x * mCellDimension.x, cellPos.y * mCellDimension.y }; }

	void select(MenuItem* item) {
		if (mSelectedItem) mSelectedItem->mIsSelected = false;
		mSelectedItem = item;
		mSelectedItem->mIsSelected = true;
	}

	void drawQuad(const hk::util::Vector2f& pos, const hk::util::Vector2f& size, const util::Color4f& color0, const util::Color4f& color1);
	void drawCellBackground(const hk::util::Vector2i& pos, const util::Color4f& color, const hk::util::Vector2i& span);

public:
	Menu() = default;
	void init(sead::Heap* heap);
	MenuItem* addText(const hk::util::Vector2i& pos, const sead::SafeString& text);
	MenuItem* addButton(const hk::util::Vector2i& pos, const sead::SafeString& text, MenuItem::FuncVoid activateFunc);

	void draw();
	void handleInput(s32 port);
	void print(const hk::util::Vector2i& pos, const util::Color4f& color, const char* str);
	void printf(const hk::util::Vector2i& pos, const util::Color4f& color, const char* fmt, ...);
	void printf(const hk::util::Vector2i& pos, const char* fmt, ...);
	static void log(const char* fmt, ...);
	void navigate(const hk::util::Vector2i& navDir);
	void activateItem();

	bool isActive() const { return mIsActive; }

	friend class MenuItem;
};

} // namespace tas
