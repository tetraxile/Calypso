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

    for (s32 i = 0; i < 5; i++) {
        addItem({ 10.0f, 500.0f + i * mFontHeight }, "item");
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
        item.draw(this, mCursorIdx);
    }

    auto* mgr = sead::ControllerMgr::instance();
    s32 port = al::getMainControllerPort();
    f32 y = 100.0f;
    printf({ 200.0f, y }, "%s", al::isValidReplayController(port) ? "valid" : "invalid");

    printf({ 10.0f, 10.0f }, "%d", mCursorIdx);

    // if (al::isPadTriggerLeft(-1)) {
    //     tas::Server* server = tas::Server::instance();
    //     s32 r = server->connect("192.168.1.16", 8171);
    //     if (r != 0)
    //         printf({ 10.0f, 10.0f }, "Connection error: %s\n", strerror(r));
    // }

    mTextWriter->endDraw();
}

void Menu::printf(const sead::Vector2f& pos, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buf[128];
    vsnprintf(buf, sizeof(buf), fmt, args);

    // drop shadow
    mTextWriter->mColor = { 0.0f, 0.0f, 0.0f, mFgColor.a };
    mTextWriter->setCursorFromTopLeft(pos + sead::Vector2f(mShadowOffset, mShadowOffset));
    mTextWriter->printImpl_(buf, -1, true, nullptr);

    // text
    mTextWriter->mColor = mFgColor;
    mTextWriter->setCursorFromTopLeft(pos);
    mTextWriter->printImpl_(buf, -1, true, nullptr);

    va_end(args);
}

void Menu::addItem(const sead::Vector2f& pos, const sead::SafeString& text) {
    MenuItem* item = new MenuItem(pos, text, mItems.size());
    mItems.pushBack(item);
}

void Menu::navigate(const sead::Vector2i& navDir) {
    mCursorIdx += navDir.y;

    mCursorIdx += mItems.size();
    mCursorIdx %= mItems.size();
}

void Menu::activateItem() {

}

MenuItem::MenuItem(const sead::Vector2f& pos, const sead::SafeString& text, s32 idx) {
    mPos = pos;
    mText = text;
    mIdx = idx;
}

void MenuItem::draw(Menu* menu, s32 cursorIdx) const {
    if (mIdx == cursorIdx)
        menu->printf(mPos, ">%s", mText.cstr());
    else
        menu->printf(mPos, " %s", mText.cstr());
}

}
