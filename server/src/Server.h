#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

#include <hk/types.h>

#include "LogWidget.h"
#include "ScriptSTAS.h"

class Server : public QObject {
	Q_OBJECT

public:
	Server(LogWidget& logWidget);
	~Server() override = default;

	void close();
	void sendFramePacket(const ScriptSTAS::Frame& frame);

#ifdef __GNUC__
	[[gnu::format(printf, 2, 3)]]
#endif
	void log(const char* fmt, ...);

signals:
	void disableScriptControls();

public slots:
	void newConnection();
	void disconnected();
	void receiveTCPData();
	void receiveUDPData();

private:
	struct PacketHeader {
		enum PacketType : u8 {
			cPacketType_None = 0,
			cPacketType_Frame,
			cPacketType_Log,
			cPacketType_Savestate,
			cPacketType_Command,
		};

		PacketHeader(PacketType type, u16 size) : type(type), size(size) {}

		PacketHeader() = delete;

		char signature[4] = { 'C', 'A', 'L', 'Y' };
		u8 version = 0;
		PacketType type = cPacketType_None;
		u16 size;
	};

	static const s32 PORT = 8171;

	QTcpServer* mTCPServer = nullptr;
	QTcpSocket* mTCPSocket = nullptr;
	QUdpSocket* mUDPSocket = nullptr;

	LogWidget& mLogWidget;
};
