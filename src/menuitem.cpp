#include "menuitem.h"
#include "menu.h"

namespace tas {

MenuItem::MenuItem(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text) : mMenu(menu), mPos(pos), mText(text) {}

void MenuItem::draw() const {
	draw_(cFgColorOn, cBgColorOff);
}

void MenuItem::draw_(const util::Color4f& fgColor, const util::Color4f& bgColor) const {
	mMenu->drawCellBackground(mPos, bgColor, mSpan);
	mMenu->printf(mPos, fgColor, " %s", mText.cstr());
}

MenuItemText::MenuItemText(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text) : MenuItem(menu, pos, text) {
	mIsSelectable = false;
}

MenuItemButton::MenuItemButton(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text, FuncVoid activateFunc) :
	MenuItem(menu, pos, text) {
	mActivateFunc = activateFunc;
}

void MenuItemButton::draw() const {
	if (mIsSelected)
		draw_(cFgColorOn, cBgColorOn);
	else
		draw_(cFgColorOff, cBgColorOff);
}

} // namespace tas
