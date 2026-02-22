#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

#include <hk/types.h>

#include "LogWidget.h"

class Server : public QObject {
	Q_OBJECT

public:
	Server(LogWidget& logWidget);
	~Server() override = default;

	void close();
#ifdef __GNUC__
	[[gnu::format(printf, 2, 3)]]
#endif
	void log(const char* fmt, ...);

public slots:
	void newConnection();
	void disconnected();
	void receiveTCPData();
	void receiveUDPData();

private:
	static const s32 PORT = 8171;

	QTcpServer* mTCPServer = nullptr;
	QTcpSocket* mTCPSocket = nullptr;
	QUdpSocket* mUDPSocket = nullptr;

	LogWidget& mLogWidget;
};
