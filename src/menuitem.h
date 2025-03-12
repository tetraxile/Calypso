#pragma once

#include <hk/util/Math.h>

#include <sead/gfx/seadColor.h>
#include <sead/prim/seadSafeString.h>

namespace tas {

class Menu;

class MenuItem {
protected:
	using ActivateFunc = void (*)();

	Menu* mMenu = nullptr;
	hk::util::Vector2i mPos = { 0, 0 };
	hk::util::Vector2i mSpan = { 1, 1 };
	sead::FixedSafeString<128> mText = sead::SafeString::cEmptyString;
	ActivateFunc mActivateFunc = nullptr;
	bool mIsSelectable = true;

public:
	MenuItem(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text);
	void draw() const;

	void setSpan(const hk::util::Vector2i& span) { mSpan = span; }

	void setText(const sead::FixedSafeString<128>& text) { mText = text; }

	void setSelectable(bool isSelectable) { mIsSelectable = isSelectable; }

	void setActivateFunc(ActivateFunc activateFunc) { mActivateFunc = activateFunc; }

	friend class Menu;
};

class MenuItemStatic : public MenuItem {
public:
	MenuItemStatic(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text);
};

class MenuItemButton : public MenuItem {
public:
	MenuItemButton(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text, ActivateFunc activateFunc);
};

} // namespace tas
