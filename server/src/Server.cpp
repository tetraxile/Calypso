#include "Server.h"

#include <QNetworkDatagram>

Server::Server(LogWidget& logWidget) : mLogWidget(logWidget) {
	mTCPServer = new QTcpServer(this);
	mTCPServer->listen(QHostAddress::Any, PORT);
	connect(mTCPServer, &QTcpServer::newConnection, this, &Server::newConnection);
	mLogWidget.log("listening for TCP connection on port %d...", PORT);

	mUDPSocket = new QUdpSocket(this);
	mUDPSocket->bind(PORT);
	connect(mUDPSocket, &QUdpSocket::readyRead, this, &Server::receiveUDPData);
	mLogWidget.log("listening for UDP packets on port %d...", PORT);
}

void Server::close() {
	if (mTCPSocket) {
		mTCPSocket->close();
	}
}

void Server::newConnection() {
	mTCPSocket = mTCPServer->nextPendingConnection();

	{
		bool isV4 = false;
		QHostAddress addrIPv6 = mTCPSocket->peerAddress();
		QHostAddress addrIPv4 = QHostAddress(addrIPv6.toIPv4Address(&isV4));
		QString addrStr = isV4 ? addrIPv4.toString() : addrIPv6.toString();
		u16 port = mTCPSocket->peerPort();
		mLogWidget.log("connection received from %s:%d", qPrintable(addrStr), port);
	}

	connect(mTCPSocket, &QTcpSocket::disconnected, mTCPSocket, &QTcpSocket::deleteLater);
	connect(mTCPSocket, &QTcpSocket::disconnected, this, &Server::disconnected);
	connect(mTCPSocket, &QTcpSocket::readyRead, this, &Server::receiveTCPData);
}

void Server::disconnected() {
	mLogWidget.log("client disconnected");
}

void Server::receiveTCPData() {
	QTextStream in(mTCPSocket);
	mLogWidget.log("[switch] %s", qPrintable(in.readAll()));
}

void Server::receiveUDPData() {
	while (mUDPSocket->hasPendingDatagrams()) {
		QNetworkDatagram datagram = mUDPSocket->receiveDatagram();
		QByteArray data = datagram.data();
		mLogWidget.log("[UDP] received %lld bytes", data.size());
		mLogWidget.log("[UDP] %s", data.data());
	}
}

void Server::log(const char* fmt, ...) {
	if (!mTCPSocket) return;

	va_list args;
	va_start(args, fmt);

	QString out = QString::vasprintf(fmt, args);

	mTCPSocket->write(qPrintable(out), out.length());

	va_end(args);
}
