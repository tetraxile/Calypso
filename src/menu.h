#ifndef MENU_H
#define MENU_H

#include <gfx/seadTextWriter.h>
#include <heap/seadDisposer.h>

namespace tas {

class Menu {
    SEAD_SINGLETON_DISPOSER(Menu);

private:
    sead::Heap* mHeap = nullptr;
    sead::TextWriter* mTextWriter = nullptr;

    sead::Color4f mFgColor = { 1.0f, 1.0f, 1.0f, 0.9f };
    sead::Color4f mBgColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    f32 mShadowOffset = 2.0f;

public:
    Menu() = default;
    void init(sead::Heap* heap);
    void draw();
    void printf(const sead::Vector2f& pos, const char* fmt, ...);

    static void initFontMgr();
};

}

#endif
