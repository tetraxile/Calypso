#include "menu.h"

#include <agl/common/aglDrawContext.h>

#include <gfx/nvn/seadDebugFontMgrNvn.h>
#include <gfx/seadViewport.h>
#include <gfx/seadTextWriter.h>

#include "al/Library/Controller/InputFunction.h"
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

    sead::TextWriter::setDefaultFont(sead::DebugFontMgrJis1Nvn::instance());

    al::DrawSystemInfo* drawInfo = Application::instance()->mDrawSystemInfo;

    agl::DrawContext* context = drawInfo->drawContext;
    const agl::RenderBuffer* renderBuffer = drawInfo->dockedRenderBuffer;

    sead::Viewport* viewport = new sead::Viewport(*renderBuffer);

    mTextWriter = new sead::TextWriter(context, viewport);
    mTextWriter->setupGraphics(context);
    mTextWriter->mColor = mFgColor;
}

void Menu::draw() {
    mTextWriter->beginDraw();
    mTextWriter->setScaleFromFontHeight(20.f);

    printf({ 10.0f, 10.0f }, "hi");

    // if (al::isPadTriggerLeft(-1)) {
    //     tas::Server* server = tas::Server::instance();
    //     s32 r = server->connect("192.168.1.16", 8171);
    //     // if (r != 0)
    //         // snprintf(menu, sizeof(menu), "Connection error: %s\n", strerror(r));
    // }

    // gTextWriter->printf(menu);

    mTextWriter->endDraw();
}

void Menu::printf(const sead::Vector2f& pos, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    f32 k = 0.25f;
    mTextWriter->mColor = { mFgColor.r * k, mFgColor.g * k, mFgColor.b * k, mFgColor.a };
    mTextWriter->setCursorFromTopLeft(pos + sead::Vector2f(mShadowOffset, mShadowOffset));
    mTextWriter->printf(fmt, args);

    mTextWriter->mColor = mFgColor;
    mTextWriter->setCursorFromTopLeft(pos);
    mTextWriter->printf(fmt, args);

    va_end(args);
}

}
