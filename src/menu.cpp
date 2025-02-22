#include "menu.h"

#include <hk/diag/diag.h>
#include <hk/util/Context.h>

#include <cstdio>

#include <nn/hid.h>

#include <agl/common/aglDrawContext.h>

#include <controller/seadControllerAddon.h>
#include <controller/seadControllerMgr.h>
#include <controller/seadControllerWrapper.h>
#include <gfx/nvn/seadDebugFontMgrNvn.h>
#include <gfx/seadViewport.h>
#include <gfx/seadTextWriter.h>
#include <heap/seadHeapMgr.h>
#include <prim/seadRuntimeTypeInfo.h>

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/PadReplayFunction.h"
#include "al/Library/File/FileUtil.h"
#include "al/Library/Memory/HeapUtil.h"
#include "al/Library/System/GameSystemInfo.h"

#include "game/System/Application.h"

constexpr static char DBG_FONT_PATH[] = "DebugData/Font/nvn_font_jis1.ntx";
constexpr static char DBG_SHADER_PATH[] = "DebugData/Font/nvn_font_shader_jis1.bin";
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
    mTextWriter->mColor = mFgColor;

    mItems = sead::PtrArray<MenuItem>();
    mItems.tryAllocBuffer(cMenuItemNumMax, heap);

    mSelectedItem = addItem({ 0, 19 }, "item");
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

void Menu::draw() {
    mTextWriter->beginDraw();
    mTextWriter->setScaleFromFontHeight(mFontHeight);

    for (auto& item : mItems) {
        item.draw();
    }

    // if (al::isPadTriggerLeft(-1)) {
    //     tas::Server* server = tas::Server::instance();
    //     s32 r = server->connect("192.168.1.16", 8171);
    //     if (r != 0)
    //         printf({ 10.0f, 10.0f }, "Connection error: %s\n", strerror(r));
    // }

    mTextWriter->endDraw();
}

void Menu::printf(const sead::Vector2i& pos, const sead::Color4f& color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    sead::Vector2f printPos = sead::Vector2f::zero;
    printPos.x = pos.x * mCellDimension.x;
    printPos.y = pos.y * mCellDimension.y;

    char buf[128];
    vsnprintf(buf, sizeof(buf), fmt, args);

    // drop shadow
    mTextWriter->mColor = { 0.0f, 0.0f, 0.0f, color.a };
    mTextWriter->setCursorFromTopLeft(printPos + sead::Vector2f(mShadowOffset, mShadowOffset));
    mTextWriter->printImpl_(buf, -1, true, nullptr);

    // text
    mTextWriter->mColor = color;
    mTextWriter->setCursorFromTopLeft(printPos);
    mTextWriter->printImpl_(buf, -1, true, nullptr);

    va_end(args);
}

void Menu::printf(const sead::Vector2i& pos, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    printf(pos, mFgColor, fmt, args);

    va_end(args);
}

MenuItem* Menu::addItem(const sead::Vector2i& pos, const sead::SafeString& text) {
    MenuItem* item = new MenuItem(this, pos, text);
    mItems.pushBack(item);
    return item;
}

void Menu::navigate(const sead::Vector2i& navDir) {
    MenuItem* nearestItem = nullptr;
    s32 nearestDist = 0;
    s32 nearestPerpDist = 0;
    
    char buf[128];
    memset(buf, 0, sizeof(buf));
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

}

MenuItem::MenuItem(Menu* menu, const sead::Vector2i& pos, const sead::SafeString& text) {
    mPos = pos;
    mText = text;
    mMenu = menu;
}

void MenuItem::draw(const sead::Color4f& color) const {
    mMenu->printf(mPos, color, "%s", mText.cstr());
}

void MenuItem::draw() const {
    if (this == mMenu->mSelectedItem)
        mMenu->printf(mPos, mMenu->mSelectedColor, ">%s", mText.cstr());
    else
        mMenu->printf(mPos, mMenu->mFgColor, " %s", mText.cstr());
}

}
