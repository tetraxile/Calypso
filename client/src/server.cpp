#include "server.h"
#include "menu.h"
#include "util.h"

#include <hk/diag/diag.h>
#include <hk/hook/Trampoline.h>

#include <cstdio>
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>
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

static constexpr int socketPoolSize = 0x600000;
static constexpr int socketAllocPoolSize = 0x20000;
char socketPool[socketPoolSize + socketAllocPoolSize] __attribute__((aligned(0x1000)));

namespace cly {

SEAD_SINGLETON_DISPOSER_IMPL(Server);

void Server::init(sead::Heap* heap) {
	mHeap = heap;
	sead::ScopedCurrentHeapSetter heapSetter(mHeap);

	nn::socket::Initialize(socketPool, socketPoolSize, socketAllocPoolSize, 0xE);

	al::FunctorV0M functor(this, &Server::threadRecv);
	mRecvThread = new (mHeap) al::AsyncFunctorThread("Recv Thread", functor, 0, 0x20000, {});

	disableSocketInit.installAtSym<"_ZN2nn6socket10InitializeEPvmmi">();
}

s32 Server::recvAll(u8* recvBuf, s32 remaining) {
	s32 totalReceived = 0;
	do {
		s32 recvLen = nn::socket::Recv(mSockFd, recvBuf, remaining, 0);
		if (recvLen <= 0) return recvLen;
		remaining -= recvLen;
		recvBuf += recvLen;
		totalReceived += recvLen;
	} while (remaining > 0);

	return totalReceived;
}

void Server::threadRecv() {
	while (true) {
		hk::Result r = handlePacket();
		if (r.failed())
			disconnect();
		nn::os::SleepThread(nn::TimeSpan::FromSeconds(1));
	}
}

hk::Result Server::handlePacket() {
	if (mState != State::CONNECTED)
		return hk::ResultSuccess();

	u8 header[cPacketHeaderSize];
	if (recvAll(header, cPacketHeaderSize) <= 0)
		return hk::ResultFailed();
	PacketType packetType = PacketType(header[0]);

	switch (packetType) {
	case PacketType::Script: {
		u8 scriptNameLen = header[1];
		u32 scriptLen = sead::Endian::swapU32(*(u32*)&header[4]);

		char scriptName[0x101];
		if (recvAll((u8*)scriptName, 0xff) <= 0)
			return hk::ResultFailed();
		scriptName[0x100] = '\0';

		log("received packet Script: name = %s, len = %d (%d)", scriptName, scriptLen);

		char filePath[0x200];
		snprintf(filePath, sizeof(filePath), "sd:/Calypso/scripts/%s", scriptName);

		util::createFile(filePath, scriptLen, true);

		nn::fs::FileHandle fileHandle;
		LOG_R(nn::fs::OpenFile(&fileHandle, filePath, nn::fs::OpenMode_Write));

		u8 chunkBuf[0x10000];
		memset(chunkBuf, 0, sizeof(chunkBuf));
		s32 totalWritten = 0;
		while (totalWritten < scriptLen) {
			s32 remaining = scriptLen - totalWritten;
			s32 chunkLen = recvAll(chunkBuf, sead::Mathf::min(remaining, sizeof(chunkBuf)));
			if (chunkLen <= 0)
				return hk::ResultFailed();
			LOG_R(nn::fs::WriteFile(fileHandle, totalWritten, chunkBuf, chunkLen, nn::fs::WriteOption::CreateOption(nn::fs::WriteOptionFlag_Flush)));
			totalWritten += chunkLen;
		}

		nn::fs::CloseFile(fileHandle);

		log("wrote %d bytes to file '%s'", totalWritten, filePath);

		Menu::log("received script '%s' (%d bytes)", scriptName, scriptLen);

		break;
	}
	case PacketType::None:
		break;
	}

	return hk::ResultSuccess();
}

s32 Server::connect(const char* serverIP, u16 port) {
	if (mState == State::CONNECTED) disconnect();

	in_addr hostAddress = { 0 };
	sockaddr_in serverAddr = { 0 };

	// create socket
	if ((mSockFd = nn::socket::Socket(AF_INET, SOCK_STREAM, 0)) < 0) return nn::socket::GetLastErrno();

	// disable Nagle's algorithm (which would delay sending packets to group smaller ones together)
	{
		const s32 i = 1;
		nn::socket::SetSockOpt(mSockFd, IPPROTO_TCP, TCP_NODELAY, &i, sizeof(i));
	}

	// configure server to connect to
	nn::socket::InetAton(serverIP, &hostAddress);
	serverAddr.sin_addr = hostAddress;
	serverAddr.sin_family = nn::socket::InetHtons(AF_INET);
	serverAddr.sin_port = nn::socket::InetHtons(port);

	// connect to server
	nn::Result result = nn::socket::Connect(mSockFd, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if (result.IsFailure()) return nn::socket::GetLastErrno();

	mState = State::CONNECTED;

	cly::Menu::log("Connected to %s:%d!", serverIP, port);

	// send message to server
	char message[] = "connected!";
	s32 r = nn::socket::Send(mSockFd, message, strlen(message), 0);
	if (r < 0) {
		disconnect();
		return nn::socket::GetLastErrno();
	}

	return 0;
}

void Server::log(const char* fmt, ...) {
	Server* server = instance();
	if (server->mState != State::CONNECTED) return;

	va_list args;
	va_start(args, fmt);

	char message[0x400];
	vsnprintf(message, sizeof(message), fmt, args);

	s32 r = nn::socket::Send(server->mSockFd, message, strlen(message), 0);

	va_end(args);
}

void Server::disconnect() {
	cly::Menu::log("disconnected from server");
	mState = State::DISCONNECTED;
	nn::socket::Close(mSockFd);
}

} // namespace cly
