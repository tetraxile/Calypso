#include "menuitem.h"
#include "menu.h"

namespace tas {

MenuItem::MenuItem(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text) : mMenu(menu), mPos(pos), mText(text) {}

void MenuItem::draw() const {
	if (this == mMenu->mSelectedItem) {
		mMenu->drawCellBackground(mPos, true, mSpan);
		mMenu->printf(mPos, mMenu->mSelectedColor, ">%s", mText.cstr());
	} else {
		mMenu->drawCellBackground(mPos, false, mSpan);
		mMenu->printf(mPos, mMenu->mFgColor, " %s", mText.cstr());
	}
}

MenuItemStatic::MenuItemStatic(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text) : MenuItem(menu, pos, text) {
	setSelectable(false);
}

MenuItemButton::MenuItemButton(Menu* menu, const hk::util::Vector2i& pos, const sead::FixedSafeString<128>& text, ActivateFunc activateFunc) :
	MenuItem(menu, pos, text) {
	setActivateFunc(activateFunc);
}

} // namespace tas
