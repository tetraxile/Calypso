#pragma once

#include <hk/gfx/DebugRenderer.h>

#include <sead/container/seadPtrArray.h>
#include <sead/gfx/seadPrimitiveRenderer.h>
#include <sead/gfx/seadTextWriter.h>
#include <sead/heap/seadDisposer.h>
#include <sead/prim/seadSafeString.h>

namespace tas {

class Menu;

class MenuItem {
private:
	using ActivateFunc = void (*)();

	Menu* mMenu = nullptr;
	hk::util::Vector2i mPos = { 0, 0 };
	hk::util::Vector2i mSpan = { 1, 1 };
	sead::FixedSafeString<128> mText = sead::SafeString::cEmptyString;
	ActivateFunc mActivateFunc = nullptr;
	bool mIsSelectable = true;

public:
	MenuItem(Menu* menu, const hk::util::Vector2i& pos, const sead::SafeString& text, ActivateFunc activateFunc = nullptr);
	void draw(const sead::Color4f& color) const;
	void draw() const;

	void setSpan(const hk::util::Vector2i& span) { mSpan = span; }

	void setText(const sead::FixedSafeString<128> text) { mText = text; }

	void setSelectable(bool isSelectable) { mIsSelectable = isSelectable; }

	friend class Menu;
};

class Menu {
	SEAD_SINGLETON_DISPOSER(Menu);

private:
	using ActivateFunc = void (*)();

	constexpr static s32 cMenuItemNumMax = 128;

	bool mIsActive = true;
	sead::Heap* mHeap = nullptr;
	hk::gfx::DebugRenderer* mRenderer = nullptr;

	const hk::util::Vector2i mScreenResolution = { 1280, 720 };
	hk::util::Vector2i mCellResolution = { 12, 36 };
	hk::util::Vector2f mCellDimension = { (float)mScreenResolution.x / mCellResolution.x, (float)mScreenResolution.y / mCellResolution.y };
	f32 mFontHeight = mCellDimension.y;

	sead::Color4f mFgColor = { 0.6f, 0.6f, 0.6f, 0.8f };
	sead::Color4f mSelectedColor = { 1.0f, 1.0f, 1.0f, 0.8f };
	sead::Color4f mBgColor0 = { 0.9f, 0.9f, 0.9f, 0.8f };
	sead::Color4f mBgColor1 = { 0.4f, 0.4f, 0.4f, 0.8f };
	f32 mShadowOffset = 2.0f;

	sead::PtrArray<MenuItem> mItems;
	MenuItem* mSelectedItem = nullptr;

	hk::util::Vector2f cellPosToAbsolute(const hk::util::Vector2i& cellPos) { return { cellPos.x * mCellDimension.x, cellPos.y * mCellDimension.y }; }

	void select(MenuItem* item) { mSelectedItem = item; }

	void drawQuad(const hk::util::Vector2f& pos, const hk::util::Vector2f& size, const sead::Color4f& color0, const sead::Color4f& color1);
	void drawCellBackground(const hk::util::Vector2i& pos, bool isSelected, const hk::util::Vector2i& span = { 1, 1 });

public:
	Menu() = default;
	void init(sead::Heap* heap);
	MenuItem* addItem(const hk::util::Vector2i& pos, const sead::SafeString& text, ActivateFunc activateFunc = nullptr);

	void draw();
	void handleInput(s32 port);
	void printf(const hk::util::Vector2i& pos, const sead::Color4f& color, const char* fmt, ...);
	void printf(const hk::util::Vector2i& pos, const char* fmt, ...);
	void navigate(const hk::util::Vector2i& navDir);
	void activateItem();

	bool isActive() const { return mIsActive; }

	friend class MenuItem;
};

} // namespace tas
