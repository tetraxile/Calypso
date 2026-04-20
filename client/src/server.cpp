#include "server.h"
#include "hk/container/Array.h"
#include "hk/types.h"
#include "menu.h"
#include "smo/sead/math/seadVectorFwd.h"
#include "util.h"

#include <hk/diag/diag.h>
#include <hk/hook/Trampoline.h>
#include <hk/svc/api.h>

#include <cstdio>
#include <cstdlib>
#include <netdb.h>
#include <netinet/tcp.h>

#include <nn/fs.h>
#include <nn/fs/fs_files.h>
#include <nn/fs/fs_types.h>
#include <nn/socket.h>
#include <sead/heap/seadDisposer.h>
#include <sead/heap/seadHeap.h>
#include <sead/heap/seadHeapMgr.h>
#include <sead/math/seadMathCalcCommon.h>
#include <sead/prim/seadEndian.h>

HkTrampoline<void> disableSocketInit = hk::hook::trampoline([]() -> void {});

static constexpr s32 socketPoolSize = 0x600000;
static constexpr s32 socketAllocPoolSize = 0x20000;
char socketPool[socketPoolSize + socketAllocPoolSize] __attribute__((aligned(0x1000)));

namespace cly {

SEAD_SINGLETON_DISPOSER_IMPL(Server);

void Server::init(sead::Heap* heap, const sead::SafeString& serverIP) {
	mHeap = heap;
	sead::ScopedCurrentHeapSetter heapSetter(mHeap);

	nn::socket::Initialize(socketPool, socketPoolSize, socketAllocPoolSize, 0xE);

	al::FunctorV0M functor(this, &Server::threadRecv);
	mRecvThread = new (mHeap) al::AsyncFunctorThread("Recv Thread", functor, 0, 0x20000, {});
	mRecvThread->start();

	disableSocketInit.installAtSym<"_ZN2nn6socket10InitializeEPvmmi">();

	HK_ABORT_UNLESS(nn::socket::InetAton(serverIP.cstr(), &mServerIP), "failed to parse server IP");

	mUDPSockFd = nn::socket::Socket(AF_INET, SOCK_DGRAM, 0);
	HK_ABORT_UNLESS(mUDPSockFd >= 0, "failed to create UDP socket");
}

s32 Server::recvAll(u8* recvBuf, s32 remaining) {
	s32 totalReceived = 0;
	do {
		s32 recvLen = nn::socket::Recv(mTCPSockFd, recvBuf, remaining, 0);
		if (recvLen < 0) {
			Menu::log("socket error: %s", strerror(nn::socket::GetLastErrno()));
			return recvLen;
		} else if (recvLen == 0) {
			Menu::log("received 0 bytes");
			return recvLen;
		}
		remaining -= recvLen;
		recvBuf += recvLen;
		totalReceived += recvLen;
	} while (remaining > 0);

	return totalReceived;
}

void Server::threadRecv() {
	while (true) {
		hk::Result r = handlePacket();
		if (r.failed()) disconnect();
		hk::svc::SleepThread(-2);
	}
}

hk::Result Server::handlePacket() {
	if (mState != State::Connected) return hk::ResultSuccess();

	Menu::log("checking for TCP packet...");

	PacketHeader header;
	if (recvAll((u8*)&header, cPacketHeaderSize) <= 0) return hk::ResultFailed();

	Menu::log("received TCP packet: type %d, size %#x", header.type, header.size);

	u8 body[header.size];
	if (recvAll(body, header.size) <= 0) return hk::ResultFailed();

	switch (header.type) {
	case PacketHeader::cPacketType_Frame: {
		FramePacket frame = *(FramePacket*)body;

		mFrameBuffer.pushBack(frame);

		Menu::log("frame buffer: %d/%d", mFrameBuffer.count, mFrameBuffer.capacity);

		break;
	}
	default: break;
	}

	return hk::ResultSuccess();
}

s32 Server::sendUDPDatagram(Server::PacketHeader::PacketType type, hk::Span<const u8> data) {
	if (mState != State::Connected) return 0;

	sockaddr_in serverAddr;
	serverAddr.sin_addr = mServerIP;
	serverAddr.sin_family = nn::socket::InetHtons(AF_INET);
	serverAddr.sin_port = nn::socket::InetHtons(cPort);

	struct [[gnu::packed]] {
		PacketHeader header;
		hk::Array<u8, 500> data;
	} message = {
		.header = { .type = type, .size = u32(data.size_bytes()) },
	};

	memcpy(message.data.data(), data.data(), data.size_bytes());

	s32 r = nn::socket::SendTo(mUDPSockFd, &message, sizeof(PacketHeader) + message.header.size, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

	if (r < 0) {
		disconnect();
		return nn::socket::GetLastErrno();
	}

	return 0;
}

s32 Server::connect() {
	if (mState == State::Connected) disconnect();

	// create socket
	if ((mTCPSockFd = nn::socket::Socket(AF_INET, SOCK_STREAM, 0)) < 0) return nn::socket::GetLastErrno();

	// disable Nagle's algorithm (which would delay sending packets to group smaller ones together)
	{
		const s32 i = 1;
		nn::socket::SetSockOpt(mTCPSockFd, IPPROTO_TCP, TCP_NODELAY, &i, sizeof(i));
	}

	// configure server to connect to
	sockaddr_in serverAddr;
	serverAddr.sin_addr = mServerIP;
	serverAddr.sin_family = nn::socket::InetHtons(AF_INET);
	serverAddr.sin_port = nn::socket::InetHtons(cPort);

	// connect to server
	nn::Result result = nn::socket::Connect(mTCPSockFd, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if (result.IsFailure()) return nn::socket::GetLastErrno();

	mState = State::Connected;

	const char* serverIP = nn::socket::InetNtoa(mServerIP);
	Menu::log("Connected to %s:%d!", serverIP, cPort);

	// send message to server
	struct {
		PacketHeader header;
		char message[11] = "connected!";
	} message = { .header = {
					  .type = PacketHeader::cPacketType_Log,
					  .size = 10,
				  } };

	s32 r = nn::socket::Send(mTCPSockFd, &message, sizeof(message) - 1, 0);
	if (r < 0) {
		disconnect();
		return nn::socket::GetLastErrno();
	}

	return 0;
}

void Server::log(const char* fmt, ...) {
	Server* server = instance();
	if (server->mState != State::Connected) return;

	va_list args;
	va_start(args, fmt);

	char message[0x400 + sizeof(PacketHeader)];
	auto packetHeader = reinterpret_cast<PacketHeader*>(message);
	s32 len = vsnprintf(message + sizeof(PacketHeader), sizeof(message) - sizeof(PacketHeader), fmt, args);
	packetHeader->type = PacketHeader::cPacketType_Log;
	packetHeader->size = len;

	s32 r = nn::socket::Send(server->mTCPSockFd, message, sizeof(PacketHeader) + len, 0);

	va_end(args);
}

void Server::reportStageName(const sead::SafeString& stageName, s32 scenarioNo) {
	Server* server = instance();
	if (server->mState != State::Connected) return;
	if (stageName == "WorldMapStage") return;

	auto nameLen = stageName.calcLength();

	struct [[gnu::packed]] {
		PacketHeader header;
		s32 scenarioNo;
		std::array<char, 0x100> stageName;
	} message = {
		.header = {
			.type = PacketHeader::cPacketType_ReportStageName,
			.size = u32(sizeof(s32) + nameLen),
		},
		.scenarioNo = scenarioNo,
	};

	memcpy(&message.stageName, stageName.cstr(), nameLen);

	s32 r = nn::socket::Send(server->mTCPSockFd, &message, sizeof(PacketHeader) + message.header.size, 0);
}

void Server::reportPlayerPosition(const sead::Vector3f& position) {
	Server* server = instance();
	if (server->mState != State::Connected) return;

	hk::Span<const sead::Vector3f> span = { &position, 1 };
	server->sendUDPDatagram(Server::PacketHeader::cPacketType_ReportPosition, cast<const u8>(span));


	// s32 r = nn::socket::Send(server->mTCPSockFd, &message, sizeof(message), 0);
}

void Server::disconnect() {
	Menu::log("disconnected from server");
	mState = State::Disconnected;
	nn::socket::Close(mTCPSockFd);
}

} // namespace cly
