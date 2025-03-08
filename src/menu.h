#ifndef MENU_H
#define MENU_H

#include <hk/gfx/DebugRenderer.h>

#include <container/seadPtrArray.h>
#include <gfx/seadTextWriter.h>
#include <gfx/seadPrimitiveRenderer.h>
#include <heap/seadDisposer.h>
#include <prim/seadSafeString.h>

namespace tas {

class Menu;

class MenuItem {
private:
    using ActivateFunc = void (*)();

    Menu* mMenu = nullptr;
    sead::Vector2i mPos = sead::Vector2i::zero;
    sead::FixedSafeString<128> mText = sead::SafeString::cEmptyString;
    ActivateFunc mActivateFunc = nullptr;

public:
    MenuItem(Menu* menu, const sead::Vector2i& pos, const sead::SafeString& text, ActivateFunc activateFunc = nullptr);
    void draw(const sead::Color4f& color) const;
    void draw() const;

    friend class Menu;
};

class Menu {
    SEAD_SINGLETON_DISPOSER(Menu);

private:
    using ActivateFunc = void (*)();

    constexpr static s32 cMenuItemNumMax = 128;

    sead::Heap* mHeap = nullptr;
    hk::gfx::DebugRenderer* mRenderer = nullptr;

    sead::Color4f mFgColor = { 0.6f, 0.6f, 0.6f, 0.8f };
    sead::Color4f mSelectedColor = { 1.0f, 1.0f, 1.0f, 0.8f };
    sead::Color4f mBgColor0 = { 0.9f, 0.9f, 0.9f, 0.8f };
    sead::Color4f mBgColor1 = { 0.4f, 0.4f, 0.4f, 0.8f };
    f32 mShadowOffset = 2.0f;
    f32 mFontHeight = 20.0f;
    hk::util::Vector2f mCellDimension = { 100.0f, mFontHeight };

    sead::PtrArray<MenuItem> mItems;
    MenuItem* mSelectedItem = nullptr;
    bool mIsActive = true;

    hk::util::Vector2f cellPosToAbsolute(const sead::Vector2i& cellPos) {
        return { cellPos.x * mCellDimension.x, cellPos.y * mCellDimension.y };
    }

    void select(MenuItem* item) { mSelectedItem = item; }
    void drawQuad(const hk::util::Vector2f& pos, const hk::util::Vector2f& size, const sead::Color4f& color0, const sead::Color4f& color1);
    void drawCellBackground(const sead::Vector2i& pos, bool isSelected);

public:
    Menu() = default;
    void init(sead::Heap* heap);
    MenuItem* addItem(const sead::Vector2i& pos, const sead::SafeString& text, ActivateFunc activateFunc = nullptr);

    void draw();
    void handleInput(s32 port);
    void printf(const sead::Vector2i& pos, const sead::Color4f& color, const char* fmt, ...);
    void printf(const sead::Vector2i& pos, const char* fmt, ...);
    void navigate(const sead::Vector2i& navDir);
    void activateItem();

    bool isActive() const { return mIsActive; }

    friend class MenuItem;
};

}

#endif
