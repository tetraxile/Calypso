#pragma once

#include "util.h"

#include <hk/util/Math.h>

#include <sead/prim/seadSafeString.h>

namespace tas {

class Menu;

class MenuItem {
protected:
	using FuncVoid = void (*)();
	using FuncSelf = void (*)(MenuItem*);

	constexpr static util::Color4f cFgColorOff = { 0.7f, 0.7f, 0.7f, 0.8f };
	constexpr static util::Color4f cFgColorOn = { 1.0f, 1.0f, 1.0f, 0.8f };
	constexpr static util::Color4f cBgColorOff = { 0.4f, 0.4f, 0.4f, 0.4f };
	constexpr static util::Color4f cBgColorOn = { 0.7f, 0.7f, 0.7f, 0.8f };

	Menu* mMenu = nullptr;
	hk::util::Vector2i mPos = { 0, 0 };
	hk::util::Vector2i mSpan = { 1, 1 };
	sead::FixedSafeString<128> mText = sead::SafeString::cEmptyString;
	FuncVoid mActivateFunc = nullptr;
	FuncSelf mUpdateFunc = nullptr;
	FuncSelf mDrawFunc = nullptr;
	bool mIsSelectable = true;
	bool mIsSelected = false;

public:
	MenuItem(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text);
	void virtual draw() const;
	void draw_(const util::Color4f& fgColor, const util::Color4f& bgColor) const;

	void setText(const sead::FixedSafeString<128>& text) { mText = text; }

	friend class Menu;
};

class MenuItemText : public MenuItem {
public:
	MenuItemText(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text);
};

class MenuItemButton : public MenuItem {
public:
	MenuItemButton(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text, FuncVoid activateFunc);
	void draw() const override;
};

} // namespace tas
