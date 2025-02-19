#include "Library/Memory/HeapUtil.h"
#include "hk/Result.h"
#include "hk/diag/diag.h"
#include "hk/hook/Trampoline.h"
#include "hk/util/Context.h"

#include <cstdio>

#include <agl/common/aglDrawContext.h>

#include <gfx/nvn/seadDebugFontMgrNvn.h>
#include <gfx/seadTextWriter.h>
#include <gfx/seadViewport.h>
#include <heap/seadHeap.h>
#include <heap/seadHeapMgr.h>

#include "al/Library/File/FileUtil.h"
#include "al/Library/Controller/InputFunction.h"
#include "al/Library/System/GameSystemInfo.h"

#include "game/System/Application.h"
#include "game/System/GameSystem.h"

#include "server.h"

using namespace hk;

sead::TextWriter* gTextWriter;

constexpr static char DBG_FONT_PATH[] = "DebugData/Font/nvn_font_jis1.ntx";
constexpr static char DBG_SHADER_PATH[] = "DebugData/Font/nvn_font_shader_jis1.bin";
constexpr static char DBG_TBL_PATH[] = "DebugData/Font/nvn_font_jis1_tbl.bin";

void initTextWriter() {
    sead::Heap* curHeap = sead::HeapMgr::instance()->getCurrentHeap();

    sead::DebugFontMgrJis1Nvn::createInstance(curHeap);

    if (al::isExistFile(DBG_SHADER_PATH) && al::isExistFile(DBG_FONT_PATH) && al::isExistFile(DBG_TBL_PATH)) {
        sead::DebugFontMgrJis1Nvn::instance()->initialize(
            curHeap, DBG_SHADER_PATH, DBG_FONT_PATH, DBG_TBL_PATH, 0x100000);
    }

    sead::TextWriter::setDefaultFont(sead::DebugFontMgrJis1Nvn::instance());

    al::DrawSystemInfo* drawInfo = Application::instance()->mDrawSystemInfo;

    agl::DrawContext* context = drawInfo->drawContext;
    const agl::RenderBuffer* renderBuffer = drawInfo->dockedRenderBuffer;

    sead::Viewport* viewport = new sead::Viewport(*renderBuffer);

    gTextWriter = new sead::TextWriter(context, viewport);
    gTextWriter->setupGraphics(context);
    gTextWriter->mColor = { 0.5f, 0.5f, 0.5f, 1.0f };
}

char menu[0x400];

HkTrampoline<void, GameSystem*> gameSystemInit = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    initTextWriter();

    sead::Heap* heap = Server::initializeHeap();
    Server* server = Server::createInstance(heap);
    server->init(heap);

    memset(menu, 0, sizeof(menu));

    gameSystemInit.orig(gameSystem);
});


HkTrampoline<void, GameSystem*> drawMainLoop = hk::hook::trampoline([](GameSystem* gameSystem) -> void {
    drawMainLoop.orig(gameSystem);

    gTextWriter->beginDraw();
    gTextWriter->setCursorFromTopLeft({ 10.0f, 10.0f });
    gTextWriter->setScaleFromFontHeight(20.f);

    if (al::isPadTriggerLeft(-1)) {
        Server* server = Server::instance();
        s32 r = server->connect("192.168.1.16", 8171);
        if (r != 0)
            snprintf(menu, sizeof(menu), "Connection error: %s\n", strerror(r));
    }

    gTextWriter->printf(menu);

    gTextWriter->endDraw();
});

extern "C" void hkMain() {
    gameSystemInit.installAtSym<"_ZN10GameSystem4initEv">();

    drawMainLoop.installAtSym<"_ZN10GameSystem8drawMainEv">();
}
