#include "menu.h"
#include "menuitem.h"
#include "server.h"
#include "util.h"

#include <hk/diag/diag.h>
#include <hk/gfx/Util.h>
#include <hk/util/Context.h>

#include <cmath>
#include <cstdio>

#include <agl/common/aglDrawContext.h>
#include <nn/hid.h>
#include <sead/controller/seadControllerAddon.h>
#include <sead/controller/seadControllerMgr.h>
#include <sead/controller/seadControllerWrapper.h>
#include <sead/gfx/seadOrthoProjection.h>
#include <sead/heap/seadHeapMgr.h>
#include <sead/prim/seadRuntimeTypeInfo.h>

#include "al/Library/Controller/InputFunction.h"
#include "al/Library/Controller/PadReplayFunction.h"
#include "al/Library/Framework/GameFrameworkNx.h"
#include "al/Library/System/GameSystemInfo.h"
#include "game/System/Application.h"

namespace cly {

SEAD_SINGLETON_DISPOSER_IMPL(Menu);

void Menu::init(sead::Heap* heap) {
	mHeap = heap;
	sead::ScopedCurrentHeapSetter heapSetter(mHeap);

	mRenderer = hk::gfx::DebugRenderer::instance();

	mItems = sead::PtrArray<MenuItem>();
	mItems.tryAllocBuffer(cMenuItemNumMax, heap);

	MenuItem* fpsCounter = addText({ mCellResolution.x - 1, 0 }, "FPS: N/A");
	fpsCounter->mDrawFunc = [](MenuItem* self) -> void {
		s32 fps = round(Application::sInstance->getGameFramework()->calcFps());
		self->mText.format("FPS: %d", fps);
		self->draw_(MenuItem::cFgColorOn, MenuItem::cBgColorOff);
	};

	MenuItem* itemConnect = addButton({ 0, 19 }, "connect", []() -> void {
		cly::Server* server = cly::Server::instance();
		s32 r = server->connect("192.168.1.215", 8171);
		if (r != 0) cly::Menu::log("Connection error: %s\n", strerror(r));
	});
	itemConnect->mSpan = { 2, 1 };
	select(itemConnect);
}

void Menu::handleInput(s32 port) {
	if (al::isPadHoldZR(port) && al::isPadTriggerZL(port)) mIsActive = !mIsActive;

	if (!isActive()) return;

	if (al::isPadTriggerUp(port)) navigate({ 0, -1 });
	if (al::isPadTriggerDown(port)) navigate({ 0, 1 });
	if (al::isPadTriggerLeft(port)) navigate({ -1, 0 });
	if (al::isPadTriggerRight(port)) navigate({ 1, 0 });
	if (al::isPadTriggerZL(port)) activateItem();
}

void Menu::navigate(const hk::util::Vector2i& navDir) {
	MenuItem* nearestItem = nullptr;
	s32 nearestDist = 0;
	s32 nearestPerpDist = 0;

	for (auto& item : mItems) {
		if (&item == mSelectedItem || !item.mIsSelectable) continue;

		hk::util::Vector2i distance = item.mPos - mSelectedItem->mPos;
		// signed distance to item along navigation direction
		s32 navDistance = navDir.x ? distance.x * navDir.x : distance.y * navDir.y;
		// perpendicular distance from navigation direction to item
		u32 perpDistance = abs(navDir.y ? distance.x : distance.y);

		// if item is not in navigation direction, discard
		if (navDistance <= 0) continue;

		// if there is not yet a nearest item...
		// or this item is closer perpendicularly than the current nearest item...
		// or this item is parallel to the current nearest item AND closer...
		if (!nearestItem || perpDistance < nearestPerpDist || (perpDistance == nearestPerpDist && navDistance < nearestDist)) {
			// ...then this is the new nearest item
			nearestItem = &item;
			nearestDist = navDistance;
			nearestPerpDist = perpDistance;
		}
	}

	if (nearestItem) select(nearestItem);
}

/*
 * ================ DRAWING ================
 */

void Menu::draw() {
	if (!isActive()) return;

	for (auto& item : mItems) {
		if (item.mUpdateFunc) item.mUpdateFunc(&item);
	}

	auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;
	mRenderer->clear();
	mRenderer->begin(drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);
	mRenderer->setGlyphHeight(mFontHeight);

	// draw menu items
	for (auto& item : mItems) {
		if (item.mDrawFunc)
			item.mDrawFunc(&item);
		else
			item.draw();
	}

	// draw log entries
	drawLog();

	mRenderer->end();
}

void Menu::drawLog() {
	for (s32 i = 0; i < cLogEntryNumMax; i++) {
		auto* entry = mLog[i];
		if (!entry) break;

		util::Color4f color = MenuItem::cFgColorOn;
		util::Color4f bgColor = MenuItem::cBgColorOff;

		// before cStartFade = full brightness
		if (entry->age < LogEntry::cStartFade) {
		}
		// between cStartFade and cEndFade = fading out
		else if (entry->age < LogEntry::cEndFade) {
			f32 fade = (f32)(entry->age - LogEntry::cStartFade) / LogEntry::cFadeLength;
			color.a = std::lerp(color.a, 0.0f, fade);
			bgColor.a = std::lerp(bgColor.a, 0.0f, fade);
		}
		// after cEndFade = fully faded out
		else {
			mLog.erase(i);
			delete entry;
			continue;
		}

		hk::util::Vector2i pos = { mCellResolution.x - 5, mCellResolution.y - 1 - i };
		drawCellBackground(pos, bgColor, { 5, 1 });
		print(pos, color, entry->text.cstr());
		entry->age++;
	}
}

/*
 * ================ UTILS ================
 */

void Menu::printAbs(const hk::util::Vector2f pos, const util::Color4f& color, const char* str) {
	// draw drop shadow first
	hk::util::Vector2f shadowPos = pos + hk::util::Vector2f(mShadowOffset, mShadowOffset);
	f32 shadowAlpha = fmax(0.0f, color.a - 0.2f);
	u32 shadowColor = hk::gfx::rgbaf(0.0f, 0.0f, 0.0f, shadowAlpha);
	mRenderer->drawString(shadowPos, str, shadowColor);

	// then draw text
	mRenderer->drawString(pos, str, color);
}

void Menu::print(const hk::util::Vector2i& pos, const util::Color4f& color, const char* str) {
	hk::util::Vector2f printPos = cellPosToAbsolute(pos);
	printAbs(printPos, color, str);
}

void Menu::printf(const hk::util::Vector2i& pos, const util::Color4f& color, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);

	char buf[128];
	vsnprintf(buf, sizeof(buf), fmt, args);

	print(pos, color, buf);

	va_end(args);
}

void Menu::printf(const hk::util::Vector2i& pos, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);

	printf(pos, MenuItem::cFgColorOff, fmt, args);

	va_end(args);
}

void Menu::log(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);

	auto& logArr = sInstance->mLog;

	auto* finalEntry = logArr[cLogEntryNumMax - 1];
	if (finalEntry) {
		logArr.erase(cLogEntryNumMax - 1);
		delete finalEntry;
	}

	auto* newEntry = new (sInstance->mHeap) LogEntry;
	newEntry->text.formatV(fmt, args);
	logArr.pushFront(newEntry);

	va_end(args);
}

MenuItem* Menu::addText(const hk::util::Vector2i& pos, const sead::SafeString& text) {
	MenuItem* item = new MenuItemText(this, pos, text);
	mItems.pushBack(item);
	return item;
}

MenuItem* Menu::addButton(const hk::util::Vector2i& pos, const sead::SafeString& text, MenuItem::FuncVoid activateFunc) {
	MenuItem* item = new MenuItemButton(this, pos, text, activateFunc);
	mItems.pushBack(item);
	return item;
}

void Menu::activateItem() {
	if (mSelectedItem && mSelectedItem->mActivateFunc) mSelectedItem->mActivateFunc();
}

void Menu::drawQuad(const hk::util::Vector2f& tl, const hk::util::Vector2f& size, const util::Color4f& color0, const util::Color4f& color1) {
	mRenderer->drawQuad(
		{ { tl.x, tl.y }, { 0, 0 }, color0 }, { { tl.x + size.x, tl.y }, { 1.0, 0 }, color0 }, { { tl.x + size.x, tl.y + size.y }, { 1.0, 1.0 }, color1 },
		{ { tl.x, tl.y + size.y }, { 0, 1.0 }, color1 }
	);
}

void Menu::drawCellBackground(const hk::util::Vector2i& pos, const util::Color4f& color, const hk::util::Vector2i& span) {
	hk::util::Vector2f size = mCellDimension * span - hk::util::Vector2f(1.0f, 1.0f);
	util::Color4f brightColor;
	brightColor.r = fmin(1.0f, color.r * 1.4f);
	brightColor.g = fmin(1.0f, color.g * 1.4f);
	brightColor.b = fmin(1.0f, color.b * 1.4f);
	brightColor.a = color.a;
	drawQuad(cellPosToAbsolute(pos), size, brightColor, color);
}

} // namespace cly
