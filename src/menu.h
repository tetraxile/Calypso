#ifndef MENU_H
#define MENU_H

#include <gfx/seadTextWriter.h>
#include <heap/seadDisposer.h>
#include <container/seadPtrArray.h>
#include <prim/seadSafeString.h>

namespace tas {

class Menu;

class MenuItem {
private:
    sead::Vector2f mPos;
    sead::FixedSafeString<128> mText;
    s32 mIdx;

public:
    MenuItem(const sead::Vector2f& pos, const sead::SafeString& text, s32 idx);
    void draw(Menu* menu, s32 cursorIdx) const;
};

class Menu {
    SEAD_SINGLETON_DISPOSER(Menu);

private:
    sead::Heap* mHeap = nullptr;
    sead::TextWriter* mTextWriter = nullptr;

    sead::Color4f mFgColor = { 1.0f, 1.0f, 1.0f, 0.9f };
    sead::Color4f mBgColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    f32 mShadowOffset = 2.0f;
    f32 mFontHeight = 20.0f;

    sead::PtrArray<MenuItem> mItems;
    s32 mCursorIdx = 0;
    bool mIsActive = true;

    constexpr static s32 cMenuItemNumMax = 128;

public:
    Menu() = default;
    void init(sead::Heap* heap);
    void addItem(const sead::Vector2f& pos, const sead::SafeString& text);

    void draw();
    void handleInput(s32 port);
    void printf(const sead::Vector2f& pos, const char* fmt, ...);
    void navigate(const sead::Vector2i& navDir);
    void activateItem();

    bool isActive() const { return mIsActive; }

    static void initFontMgr();
};

}

#endif
