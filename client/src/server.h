#pragma once

#include <hk/types.h>

#include <netinet/in.h>

#include <nn/os.h>
#include <sead/basis/seadTypes.h>
#include <sead/heap/seadExpHeap.h>

#include "al/Library/Thread/AsyncFunctorThread.h"

namespace cly {

class Server {
	SEAD_SINGLETON_DISPOSER(Server);

private:
	enum class State {
		UNINITIALIZED,
		DISCONNECTED,
		CONNECTED,
	};

	enum class PacketType : u8 {
		None = 0x00,
		Script = 0x01,
	};

	constexpr static s32 cPacketHeaderSize = 0x10;
	constexpr static s32 cPort = 8171;

	sead::Heap* mHeap = nullptr;
	al::AsyncFunctorThread* mRecvThread = nullptr;
	void* mThreadStack = nullptr;
	in_addr mServerIP;
	s32 mTCPSockFd = -1; // for receiving script data, sending logs
	s32 mUDPSockFd = -1; // for sending real-time game info/inputs
	State mState = State::UNINITIALIZED;
	PacketType mCurPacketType = PacketType::None;

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
