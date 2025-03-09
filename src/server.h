#ifndef SERVER_H
#define SERVER_H

#include <nn/os.h>
#include <sead/basis/seadTypes.h>
#include <sead/heap/seadExpHeap.h>

#include "al/Library/Thread/AsyncFunctorThread.h"

namespace tas {

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

	sead::Heap* mHeap = nullptr;
	al::AsyncFunctorThread* mRecvThread = nullptr;
	void* mThreadStack = nullptr;
	s32 mSockFd = -1;
	State mState = State::UNINITIALIZED;
	PacketType mCurPacketType = PacketType::None;

	void threadRecv();
	void handlePacket();
	s32 recvAll(u8* recvBuf, s32 remaining);

public:
	Server() = default;

	void init(sead::Heap* heap);
	s32 connect(const char* serverIP, u16 port);

	static void log(const char* fmt, ...);
};

} // namespace tas

#endif
