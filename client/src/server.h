#pragma once

#include <hk/os/Event.h>
#include <hk/types.h>
#include <hk/util/Math.h>

#include <netinet/in.h>

#include <nn/os.h>
#include <sead/basis/seadTypes.h>
#include <sead/container/seadRingBuffer.h>
#include <sead/heap/seadExpHeap.h>

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

	struct PacketHeader {
		enum PacketType : u8 {
			cPacketType_None = 0,
			cPacketType_Frame,
			cPacketType_Log,
			cPacketType_Savestate,
			cPacketType_Command,
		};

		char signature[4];
		u8 version;
		PacketType type;
		u16 size;
	};

	struct Controller {
		u64 flags;
		u64 buttons;
		hk::util::Vector2i leftStick;
		hk::util::Vector2i rightStick;
		hk::util::Vector3f accelLeft;
		hk::util::Vector3f gyroLeft;
		hk::util::Vector3f accelRight;
		hk::util::Vector3f gyroRight;
	};

	struct FramePacket {
		u32 flags;
		u32 frameIdx;
		Controller player1;
		Controller player2;
		u64 amiibo;
	};

	constexpr static s32 cPacketHeaderSize = 0x8;
	constexpr static s32 cPort = 8171;

	sead::Heap* mHeap = nullptr;
	al::AsyncFunctorThread* mRecvThread = nullptr;
	void* mThreadStack = nullptr;
	in_addr mServerIP;
	s32 mTCPSockFd = -1; // for receiving script data, sending logs
	s32 mUDPSockFd = -1; // for sending real-time game info/inputs
	State mState = State::Uninitialised;

	struct {
		s32 count = 0;
		s32 capacity = 60;
		s32 head = 0;
		FramePacket buf[60];
		hk::os::Event removeEvent;

		void pushBack(FramePacket& frame) {
			if (count > capacity) removeEvent.wait();

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
			removeEvent.signal();
			return frame;
		}
	} mFrameBuffer;

	void threadRecv();
	hk::Result handlePacket();
	s32 recvAll(u8* recvBuf, s32 remaining);

public:
	Server() = default;

	void init(sead::Heap* heap, const sead::SafeString& serverIP);
	s32 connect();
	void disconnect();
	s32 sendUDPDatagram();

	static void log(const char* fmt, ...);
};

} // namespace cly
