#include "menu.h"
#include "hk/gfx/Util.h"
#include "server.h"

#include <hk/diag/diag.h>
#include <hk/util/Context.h>

#include <cstdio>

#include <nn/hid.h>

#include <agl/common/aglDrawContext.h>

#include <controller/seadControllerAddon.h>
#include <controller/seadControllerMgr.h>
#include <controller/seadControllerWrapper.h>
#include <gfx/nvn/seadDebugFontMgrNvn.h>
#include <gfx/seadOrthoProjection.h>
#include <gfx/seadPrimitiveRenderer.h>
#include <gfx/seadTextWriter.h>
#include <gfx/seadViewport.h>
#include <heap/seadHeapMgr.h>
#include <prim/seadRuntimeTypeInfo.h>

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/PadReplayFunction.h"
#include "al/Library/System/GameSystemInfo.h"

#include "game/System/Application.h"

namespace tas {

SEAD_SINGLETON_DISPOSER_IMPL(Menu);

void Menu::init(sead::Heap* heap) {
    mHeap = heap;
    sead::ScopedCurrentHeapSetter heapSetter(mHeap);

    mRenderer = hk::gfx::DebugRenderer::instance();

    mItems = sead::PtrArray<MenuItem>();
    mItems.tryAllocBuffer(cMenuItemNumMax, heap);

    mSelectedItem = addItem({ 0, 19 }, "connect", []() -> void {
        tas::Server* server = tas::Server::instance();
        s32 r = server->connect("192.168.1.215", 8171);
        // tas::Menu* menu = tas::Menu::instance();
        // if (r != 0)
        //     menu->printf({ 5, 30 }, "Connection error: %s\n", strerror(r));
    });
    for (s32 x = 0; x < 3; x++) {
        for (s32 y = 0; y < 5; y++) {
            addItem({ x, 20 + y }, "item");
        }
    }
}

void Menu::handleInput(s32 port) {
    if (!isActive()) return;

    if (al::isPadTriggerUp(port))
        navigate({ 0, -1 });
    if (al::isPadTriggerDown(port))
        navigate({ 0, 1 });
    if (al::isPadTriggerLeft(port))
        navigate({ -1, 0 });
    if (al::isPadTriggerRight(port))
        navigate({ 1, 0 });
    if (al::isPadTriggerA(port))
        activateItem();
}

void Menu::drawQuad(const hk::util::Vector2f& tl, const hk::util::Vector2f& size, const sead::Color4f& color0, const sead::Color4f& color1) {
    u32 uColor0 = hk::gfx::rgbaf(color0.r, color0.g, color0.b, color0.a);
    u32 uColor1 = hk::gfx::rgbaf(color1.r, color1.g, color1.b, color1.a);

    mRenderer->drawQuad(
        { { tl.x, tl.y }, { 0, 0 }, uColor0 },
        { { tl.x + size.x, tl.y }, { 1.0, 0 }, uColor0 },
        { { tl.x + size.x, tl.y + size.y }, { 1.0, 1.0 }, uColor1 },
        { { tl.x, tl.y + size.y }, { 0, 1.0 }, uColor1 }
    );
}

void Menu::drawCellBackground(const sead::Vector2i& pos, bool isSelected) {
    f32 shade = isSelected ? 1.0f : 0.5f;
    drawQuad(cellPosToAbsolute(pos), mCellDimension - hk::util::Vector2f(1.0f, 1.0f), mBgColor0 * shade, mBgColor1 * shade);
}

void Menu::draw() {
    auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;
    mRenderer->clear();
    mRenderer->begin(drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);
    mRenderer->setGlyphHeight(mFontHeight);

    for (auto& item : mItems) {
        item.draw();
    }
    
    mRenderer->end();
}

void Menu::printf(const sead::Vector2i& pos, const sead::Color4f& color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    hk::util::Vector2f printPos = cellPosToAbsolute(pos);

    char buf[128];
    vsnprintf(buf, sizeof(buf), fmt, args);

    // draw drop shadow first
    hk::util::Vector2f shadowPos = printPos + hk::util::Vector2f(mShadowOffset, mShadowOffset);
    u32 shadowColor = hk::gfx::rgbaf(0.0f, 0.0f, 0.0f, color.a * 0.8f);
    mRenderer->drawString(shadowPos, buf, shadowColor);

    // then draw text
    u32 textColor = hk::gfx::rgbaf(color.r, color.g, color.b, color.a);
    mRenderer->drawString(printPos, buf, textColor);

    va_end(args);
}

void Menu::printf(const sead::Vector2i& pos, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    printf(pos, mFgColor, fmt, args);

    va_end(args);
}

MenuItem* Menu::addItem(const sead::Vector2i& pos, const sead::SafeString& text, ActivateFunc activateFunc) {
    MenuItem* item = new MenuItem(this, pos, text, activateFunc);
    mItems.pushBack(item);
    return item;
}

void Menu::navigate(const sead::Vector2i& navDir) {
    MenuItem* nearestItem = nullptr;
    s32 nearestDist = 0;
    s32 nearestPerpDist = 0;
    
    for (auto& item : mItems) {
        if (&item == mSelectedItem) continue;

        sead::Vector2i distance = item.mPos - mSelectedItem->mPos;
        s32 navDistance = navDir.x ? distance.x * navDir.x : distance.y * navDir.y;
        s32 perpDistance = sead::Mathf::abs(navDir.y ? distance.x : distance.y);
        if (navDistance > 0 && (!nearestItem || perpDistance < nearestPerpDist || (navDistance < nearestDist && perpDistance == nearestPerpDist))) {
            nearestItem = &item;
            nearestDist = navDistance;
            nearestPerpDist = perpDistance;
        }
    }

    if (nearestItem)
        select(nearestItem);
}

void Menu::activateItem() {
    if (mSelectedItem && mSelectedItem->mActivateFunc)
        mSelectedItem->mActivateFunc();
}

MenuItem::MenuItem(Menu* menu, const sead::Vector2i& pos, const sead::SafeString& text, ActivateFunc activateFunc) : mMenu(menu), mPos(pos), mText(text), mActivateFunc(activateFunc) {}

void MenuItem::draw(const sead::Color4f& color) const {
    mMenu->drawCellBackground(mPos, this == mMenu->mSelectedItem);
    mMenu->printf(mPos, color, "%s", mText.cstr());
}

void MenuItem::draw() const {
    if (this == mMenu->mSelectedItem) {
        mMenu->drawCellBackground(mPos, true);
        mMenu->printf(mPos, mMenu->mSelectedColor, ">%s", mText.cstr());
    } else {
        mMenu->drawCellBackground(mPos, false);
        mMenu->printf(mPos, mMenu->mFgColor, " %s", mText.cstr());
    }
}

}
