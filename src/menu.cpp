#include "menu.h"
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
#include "al/Library/File/FileUtil.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/System/GameSystemInfo.h"

#include "game/System/Application.h"
#include "gfx/seadCamera.h"

constexpr static char DBG_SHADER_PATH[] = "DebugData/Font/nvn_font_shader_jis1.bin";
constexpr static char DBG_FONT_PATH[] = "DebugData/Font/nvn_font_jis1.ntx";
constexpr static char DBG_TBL_PATH[] = "DebugData/Font/nvn_font_jis1_tbl.bin";

namespace tas {

SEAD_SINGLETON_DISPOSER_IMPL(Menu);

void Menu::initFontMgr() {
    sead::Heap* heap = al::getStationedHeap();

    sead::DebugFontMgrJis1Nvn::createInstance(heap);

    if (al::isExistFile(DBG_SHADER_PATH) && al::isExistFile(DBG_FONT_PATH) && al::isExistFile(DBG_TBL_PATH)) {
        sead::DebugFontMgrJis1Nvn::instance()->initialize(heap, DBG_SHADER_PATH, DBG_FONT_PATH, DBG_TBL_PATH, 0x100000);
    }
}

void Menu::init(sead::Heap* heap) {
    mHeap = heap;

    sead::ScopedCurrentHeapSetter heapSetter(mHeap);

    sead::TextWriter::setDefaultFont(sead::DebugFontMgrJis1Nvn::instance());

    al::DrawSystemInfo* drawInfo = Application::instance()->mDrawSystemInfo;

    agl::DrawContext* context = drawInfo->drawContext;
    const agl::RenderBuffer* renderBuffer = drawInfo->dockedRenderBuffer;

    sead::Viewport* viewport = new sead::Viewport(*renderBuffer);

    mTextWriter = new sead::TextWriter(context, viewport);
    mTextWriter->setupGraphics(context);
    mTextWriter->setScaleFromFontHeight(mFontHeight);
    mTextWriter->mColor = mFgColor;

    mItems = sead::PtrArray<MenuItem>();
    mItems.tryAllocBuffer(cMenuItemNumMax, heap);

    mSelectedItem = addItem({ 0, 19 }, "connect", []() -> void {
        tas::Server* server = tas::Server::instance();
        tas::Menu* menu = tas::Menu::instance();
        s32 r = server->connect("192.168.1.16", 8171);
        // if (r != 0)
        //     menu->printf({ 5, 30 }, "Connection error: %s\n", strerror(r));
    });
    for (s32 x = 0; x < 3; x++) {
        for (s32 y = 0; y < 5; y++) {
            addItem({ x, 20 + y }, "item");
        }
    }

    auto* projection = new sead::OrthoProjection;
    projection->setByViewport(*viewport);
    auto* camera = new sead::OrthoCamera(*projection);

    mDrawer = new sead::PrimitiveDrawer(context);
    mDrawer->setCamera(camera);
    mDrawer->setProjection(projection);
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

void Menu::drawQuad(const sead::Vector2f& tl, const sead::Vector2f& size, const sead::Color4f& color0, const sead::Color4f& color1) {
    sead::Vector2f screen(1280.0f, 720.0f);
    sead::Vector3f pos(-screen.x / 2 + size.x / 2 + tl.x, screen.y / 2 - size.y / 2 - tl.y, 0.0f);
    mDrawer->begin();
    mDrawer->drawQuad({ pos, size, color0, color1 });
    mDrawer->end();
}

void Menu::drawCellBackground(const sead::Vector2i& pos, bool isSelected) {
    f32 shade = isSelected ? 1.0f : 0.5f;
    drawQuad(cellPosToAbsolute(pos), mCellDimension - sead::Vector2f(1.0f, 1.0f), mBgColor0 * shade, mBgColor1 * shade);
}

void Menu::draw() {
    for (auto& item : mItems) {
        item.draw();
    }
}

void Menu::printf(const sead::Vector2i& pos, const sead::Color4f& color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    sead::Vector2f printPos = cellPosToAbsolute(pos);

    char buf[128];
    vsnprintf(buf, sizeof(buf), fmt, args);

    mTextWriter->beginDraw();

    // draw drop shadow first
    mTextWriter->mColor = { 0.0f, 0.0f, 0.0f, color.a };
    mTextWriter->setCursorFromTopLeft(printPos + sead::Vector2f(mShadowOffset, mShadowOffset));
    mTextWriter->printImpl_(buf, -1, true, nullptr);

    // then draw text
    mTextWriter->mColor = color;
    mTextWriter->setCursorFromTopLeft(printPos);
    mTextWriter->printImpl_(buf, -1, true, nullptr);

    mTextWriter->endDraw();

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
