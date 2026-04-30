#pragma once

#include <hk/container/Span.h>
#include <hk/os/Event.h>
#include <hk/types.h>
#include <hk/util/Math.h>

#include <netinet/in.h>

#include <nn/os.h>
#include <sead/basis/seadTypes.h>
#include <sead/container/seadRingBuffer.h>
#include <sead/heap/seadExpHeap.h>
#include <sead/math/seadVector.h>

#include "al/Library/Thread/AsyncFunctorThread.h"

namespace cly {

class Server {
	SEAD_SINGLETON_DISPOSER(Server);

private:
	enum class State {
		Uninitialised,
		Disconnected,
		Connected,
	};

	struct [[gnu::packed]] PacketHeader {
		enum PacketType : u8 {
			cPacketType_ServerInfo,
			cPacketType_ScriptInfo,
			cPacketType_Frame,
			cPacketType_Log,
			cPacketType_LoadSave,
			cPacketType_GetSave,
			cPacketType_ReportStageName,
			cPacketType_ReportPosition,
			cPacketType_PauseGame,
			cPacketType_AdvanceFrame,
			cPacketType_UDPDiscovery,
			cPacketType_FullFrameBuffer,
			cPacketType_StartScript,
			cPacketType_StopScript,
			cPacketType_ScriptEnded,
		};

		PacketType type;
		u32 size;
	};

	struct ServerInfo {
		u16 version;
	};

public:
	struct Controller {
		u64 buttons;
		sead::Vector2i leftStick;
		sead::Vector2i rightStick;
		sead::Vector3f accelLeft;
		sead::Vector3f gyroLeft;
		sead::Vector3f accelRight;
		sead::Vector3f gyroRight;
	};

	struct FramePacket {
		u32 frameIndex;
		u32 serverIndex;
		Controller player1;
		Controller player2;
		u64 amiibo;
	};

	struct ScriptInfoPacket {
		u32 frameCount;
		u8 playerCount;
		u8 controllerTypes[2];
	};

private:
	constexpr static s32 cPort = 8171;

	sead::Heap* mHeap = nullptr;
	al::AsyncFunctorThread* mRecvThread = nullptr;
	void* mThreadStack = nullptr;
	in_addr mServerIP;
	in_addr mBroadcastIP;
	s32 mTCPSockFd = -1; // for receiving script data, sending logs
	s32 mUDPSockFd = -1; // for sending real-time game info/inputs
	State mState = State::Uninitialised;

	void threadRecv();
	hk::Result handlePacket();
	s32 recvAll(u8* recvBuf, s32 remaining);

public:
	Server() = default;

	void init(sead::Heap* heap);
	s32 connect();
	void disconnect();
	bool sendTCPMessage(hk::Span<const u8> data);

	template <typename T>
	bool sendTCPMessage(T& message) {
		return sendTCPMessage(hk::Span { cast<const u8*>(&message), sizeof(PacketHeader) + message.header.size });
	}

	s32 sendUDPDatagram(PacketHeader::PacketType type, hk::Span<const u8> data);
	void sendUDPDiscoveryBroadcast();

	static void log(const char* fmt, ...);
	static void reportStageName(const sead::SafeString& stageName, s32 scenarioNo);
	static void reportPlayerPosition(const sead::Vector3f& position);
	static void reportScriptCompleted();

	struct {
		s32 count = 0;
		s32 capacity = 60;
		s32 head = 0;
		FramePacket buf[60];

		void clear() {
			count = 0;
		}

		void pushBack(FramePacket& frame) {
			buf[head++] = frame;
			count++;
			if (head >= capacity) head -= capacity;
		}

		hk::ValueOrResult<FramePacket> popBack() {
			if (count <= 0) return hk::ResultNoValue();

			count--;
			head--;
			if (head < 0) head += capacity;
			FramePacket frame = buf[head];
			return frame;
		}
	} mFrameBuffer;
};

} // namespace cly
