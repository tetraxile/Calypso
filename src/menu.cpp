#include "menu.h"
#include "menuitem.h"
#include "server.h"

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

	MenuItem* itemConnect = addButton({ 0, 19 }, "connect", []() -> void {
		tas::Server* server = tas::Server::instance();
		s32 r = server->connect("192.168.1.215", 8171);
		if (r != 0) tas::Menu::log("Connection error: %s\n", strerror(r));
	});
	itemConnect->setSpan({ 3, 1 });
	mSelectedItem = itemConnect;

	addButton({ 0, 20 }, "a", []() -> void { log("pressed button a"); });
	addButton({ 1, 20 }, "b", []() -> void { log("pressed button b"); });
	addButton({ 2, 20 }, "c", []() -> void { log("pressed button c"); });
}

void Menu::handleInput(s32 port) {
	if (!isActive()) return;

	if (al::isPadTriggerUp(port)) navigate({ 0, -1 });
	if (al::isPadTriggerDown(port)) navigate({ 0, 1 });
	if (al::isPadTriggerLeft(port)) navigate({ -1, 0 });
	if (al::isPadTriggerRight(port)) navigate({ 1, 0 });
	if (al::isPadTriggerZL(port)) activateItem();
}

void Menu::drawQuad(const hk::util::Vector2f& tl, const hk::util::Vector2f& size, const sead::Color4f& color0, const sead::Color4f& color1) {
	u32 uColor0 = hk::gfx::rgbaf(color0.r, color0.g, color0.b, color0.a);
	u32 uColor1 = hk::gfx::rgbaf(color1.r, color1.g, color1.b, color1.a);

	mRenderer->drawQuad(
		{ { tl.x, tl.y }, { 0, 0 }, uColor0 }, { { tl.x + size.x, tl.y }, { 1.0, 0 }, uColor0 }, { { tl.x + size.x, tl.y + size.y }, { 1.0, 1.0 }, uColor1 },
		{ { tl.x, tl.y + size.y }, { 0, 1.0 }, uColor1 }
	);
}

void Menu::drawCellBackground(const hk::util::Vector2i& pos, bool isSelected, const hk::util::Vector2i& span) {
	f32 shade = isSelected ? 1.0f : 0.5f;
	hk::util::Vector2f size = mCellDimension * span - hk::util::Vector2f(1.0f, 1.0f);
	drawQuad(cellPosToAbsolute(pos), size, mBgColor0 * shade, mBgColor1 * shade);
}

void Menu::draw() {
	auto* drawContext = Application::instance()->mDrawSystemInfo->drawContext;
	mRenderer->clear();
	mRenderer->begin(drawContext->getCommandBuffer()->ToData()->pNvnCommandBuffer);
	mRenderer->setGlyphHeight(mFontHeight);

	// for (s32 y = 0; y < mCellResolution.y; y++) {
	// 	for (s32 x = 0; x < mCellResolution.x; x++) {
	// 		drawCellBackground({ x, y }, false);
	// 	}
	// }

	for (auto& item : mItems) {
		item.draw();
	}

	for (s32 i = 0; i < cLogEntryNumMax; i++) {
		auto* entry = mLog[i];
		if (!entry) break;

		sead::Color4f color = mSelectedColor;
		if (entry->age < LogEntry::cStartFade) {
			// before cStartFade = full brightness
		} else if (entry->age < LogEntry::cEndFade) {
			// between cStartFade and cEndFade = fading out
			f32 fade = (f32)(entry->age - LogEntry::cStartFade) / LogEntry::cFadeLength;
			color.a = std::lerp(color.a, 0.0f, fade);
		} else {
			// after cEndFade = fully faded out
			mLog.erase(i);
			delete entry;
			continue;
		}

		hk::util::Vector2i pos = { mCellResolution.x - 5, mCellResolution.y - 1 - i };
		drawCellBackground(pos, false, { 5, 1 });
		print(pos, color, entry->text.cstr());
		entry->age++;
	}

	mRenderer->end();
}

void Menu::print(const hk::util::Vector2i& pos, const sead::Color4f& color, const char* str) {
	hk::util::Vector2f printPos = cellPosToAbsolute(pos);

	// draw drop shadow first
	hk::util::Vector2f shadowPos = printPos + hk::util::Vector2f(mShadowOffset, mShadowOffset);
	u32 shadowColor = hk::gfx::rgbaf(0.0f, 0.0f, 0.0f, color.a * 0.8f);
	mRenderer->drawString(shadowPos, str, shadowColor);

	// then draw text
	u32 textColor = hk::gfx::rgbaf(color.r, color.g, color.b, color.a);
	mRenderer->drawString(printPos, str, textColor);
}

void Menu::printf(const hk::util::Vector2i& pos, const sead::Color4f& color, const char* fmt, ...) {
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

	printf(pos, mFgColor, fmt, args);

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

MenuItem* Menu::addStatic(const hk::util::Vector2i& pos, const sead::SafeString& text) {
	MenuItem* item = new MenuItemStatic(this, pos, text);
	mItems.pushBack(item);
	return item;
}

MenuItem* Menu::addButton(const hk::util::Vector2i& pos, const sead::SafeString& text, ActivateFunc activateFunc) {
	MenuItem* item = new MenuItemButton(this, pos, text, activateFunc);
	mItems.pushBack(item);
	return item;
}

void Menu::navigate(const hk::util::Vector2i& navDir) {
	MenuItem* nearestItem = nullptr;
	s32 nearestDist = 0;
	s32 nearestPerpDist = 0;

	for (auto& item : mItems) {
		if (&item == mSelectedItem || !item.mIsSelectable) continue;

		hk::util::Vector2i distance = item.mPos - mSelectedItem->mPos;
		s32 navDistance = navDir.x ? distance.x * navDir.x : distance.y * navDir.y;
		s32 perpDistance = abs(navDir.y ? distance.x : distance.y);
		if (navDistance > 0 && (!nearestItem || perpDistance < nearestPerpDist || (navDistance < nearestDist && perpDistance == nearestPerpDist))) {
			nearestItem = &item;
			nearestDist = navDistance;
			nearestPerpDist = perpDistance;
		}
	}

	if (nearestItem) select(nearestItem);
}

void Menu::activateItem() {
	if (mSelectedItem && mSelectedItem->mActivateFunc) mSelectedItem->mActivateFunc();
}

} // namespace tas
