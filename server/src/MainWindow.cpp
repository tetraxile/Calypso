#include "MainWindow.h"

#include <QCoreApplication>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QNetworkDatagram>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTableWidget>
#include <QVector3D>

#include "types.h"
#include "util.h"

void MainWindow::newConnection() {
	mTCPSocket = mServer->nextPendingConnection();

	{
		bool isV4 = false;
		QHostAddress addrIPv6 = mTCPSocket->peerAddress();
		QHostAddress addrIPv4 = QHostAddress(addrIPv6.toIPv4Address(&isV4));
		QString addrStr = isV4 ? addrIPv4.toString() : addrIPv6.toString();
		u16 port = mTCPSocket->peerPort();
		log("connection received from %s:%d", qPrintable(addrStr), port);
	}

	connect(mTCPSocket, &QTcpSocket::disconnected, mTCPSocket, &QTcpSocket::deleteLater);
	connect(mTCPSocket, &QTcpSocket::disconnected, this, &MainWindow::disconnected);
	connect(mTCPSocket, &QTcpSocket::readyRead, this, &MainWindow::receiveTCPData);
}

void MainWindow::disconnected() {
	log("client disconnected");
}

void MainWindow::receiveTCPData() {
	QTextStream in(mTCPSocket);
	log("[switch] %s", qPrintable(in.readAll()));
}

void MainWindow::receiveUDPData() {
	while (mUDPSocket->hasPendingDatagrams()) {
		QNetworkDatagram datagram = mUDPSocket->receiveDatagram();
		QByteArray data = datagram.data();
		log("[UDP] received %lld bytes", data.size());
		log("[UDP] %s", data.data());
	}
}

void MainWindow::openFileButton() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select a script"), "", tr("STAS scripts (*.stas)"));
	if (fileName.isEmpty()) return;

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		log("could not open file: %s", qPrintable(fileName));
		return;
	}

	log("opening file: %s", qPrintable(fileName));
	if (mScript) delete mScript;
	mScript = new ScriptSTAS(file);

	mRecentScripts->insertItem(0, fileName);
	mRecentScripts->setCurrentIndex(0);
}

void MainWindow::openFileRecent() {
	QString fileName = mRecentScripts->currentText();

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		log("could not open file: %s", qPrintable(fileName));
		return;
	}

	log("opening file: %s", qPrintable(fileName));
	if (mScript) delete mScript;
	mScript = new ScriptSTAS(file);
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
	setupUi();

	mServer = new QTcpServer(this);
	mServer->listen(QHostAddress::Any, PORT);
	connect(mServer, &QTcpServer::newConnection, this, &MainWindow::newConnection);
	log("listening for TCP connection on port %d...", PORT);

	mUDPSocket = new QUdpSocket(this);
	mUDPSocket->bind(PORT);
	connect(mUDPSocket, &QUdpSocket::readyRead, this, &MainWindow::receiveUDPData);
	log("listening for UDP packets on port %d...", PORT);
}

void MainWindow::closeEvent(QCloseEvent* event) {
	if (mTCPSocket) mTCPSocket->close();
	mServer->close();

	event->accept();
}

MainWindow::~MainWindow() {
	// delete mViewport;
}

void MainWindow::setupUi() {
	if (objectName().isEmpty()) setObjectName("MainWindow");
	resize(1280, 720);

	QWidget* centralWidget = new QWidget(this);
	setCentralWidget(centralWidget);

	QVBoxLayout* vboxLayout = new QVBoxLayout(centralWidget);
	mTopRow = new QHBoxLayout;
	mBottomRow = new QHBoxLayout;
	vboxLayout->addLayout(mTopRow);
	vboxLayout->addLayout(mBottomRow);

	setupControls();
	setupSavestates();
	setupLog();
	setupInputDisplay();
	setupGameInfo();

	mMenuBar = new QMenuBar(this);
	setMenuBar(mMenuBar);
	QMenu* menuMeow = new QMenu(mMenuBar);
	mMenuBar->addAction(menuMeow->menuAction());

	mStatusBar = new QStatusBar(this);
	setStatusBar(mStatusBar);

	retranslateUi();

	QMetaObject::connectSlotsByName(this);
}

void MainWindow::retranslateUi() {
	setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
	// openScriptButton->setText(QString());
	// playButton->setText(QCoreApplication::translate("MainWindow", "Play script", nullptr));
	// stopButton->setText(QCoreApplication::translate("MainWindow", "Stop script", nullptr));
	// togglePauseButton->setText(QCoreApplication::translate("MainWindow", "Toggle game pause", nullptr));
	// frameAdvanceButton->setText(QCoreApplication::translate("MainWindow", "Frame advance", nullptr));
	// menuMeow->setTitle(QCoreApplication::translate("MainWindow", "Meow", nullptr));
}

void MainWindow::addSection(QFrame* section, const QString& name, QHBoxLayout* row) {
	QVBoxLayout* vboxLayout = new QVBoxLayout;
	row->addLayout(vboxLayout);

	QLabel* label = new QLabel(name);
	label->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
	vboxLayout->addWidget(label);

	section->setFrameShape(QFrame::StyledPanel);
	vboxLayout->addWidget(section);
}

void MainWindow::setupControls() {
	QFrame* controlsWidget = new QFrame;
	addSection(controlsWidget, tr("Controls"), mTopRow);

	QVBoxLayout* controlsLayout = new QVBoxLayout(controlsWidget);
	// {
	//     QFormLayout* row = new QFormLayout;
	//     controlsLayout->addLayout(row);
	//     QLineEdit* switchIPWidget = new QLineEdit;
	//     switchIPWidget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
	//     row->addRow("Switch IP", switchIPWidget);
	// }
	{
		QHBoxLayout* row = new QHBoxLayout;
		controlsLayout->addLayout(row);

		mRecentScripts = new QComboBox;
		connect(mRecentScripts, &QComboBox::textActivated, this, &MainWindow::openFileRecent);
		mRecentScripts->setMaxCount(RECENT_SCRIPTS_NUM);
		row->addWidget(mRecentScripts);

		QPushButton* openButton = new QPushButton(tr("Open"));
		connect(openButton, &QPushButton::clicked, this, &MainWindow::openFileButton);
		openButton->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
		row->addWidget(openButton);
	}
	{
		QHBoxLayout* row = new QHBoxLayout;
		controlsLayout->addLayout(row);

		QPushButton* playButton = new QPushButton(tr("Run script"));
		playButton->setDisabled(true);
		row->addWidget(playButton);

		QPushButton* stopButton = new QPushButton(tr("Stop script"));
		stopButton->setDisabled(true);
		row->addWidget(stopButton);
	}
	{
		QHBoxLayout* row = new QHBoxLayout;
		controlsLayout->addLayout(row);

		QPushButton* pauseButton = new QPushButton(tr("Toggle game pause"));
		pauseButton->setDisabled(true);
		row->addWidget(pauseButton);

		QPushButton* frameAdvanceButton = new QPushButton(tr("Frame advance"));
		frameAdvanceButton->setDisabled(true);
		row->addWidget(frameAdvanceButton);
	}
	{
		QFormLayout* form = new QFormLayout;
		controlsLayout->addLayout(form);

		mScriptInfo.name = new QLineEdit;
		mScriptInfo.name->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
		mScriptInfo.name->setReadOnly(true);
		form->addRow(tr("Script name"), mScriptInfo.name);

		mScriptInfo.author = new QLineEdit;
		mScriptInfo.author->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
		mScriptInfo.author->setReadOnly(true);
		mScriptInfo.author->setCursorPosition(0);
		form->addRow(tr("Author"), mScriptInfo.author);

		mScriptInfo.commandCount = new QLineEdit;
		mScriptInfo.commandCount->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
		mScriptInfo.commandCount->setReadOnly(true);
		form->addRow(tr("Command count"), mScriptInfo.commandCount);
	}
}

void MainWindow::setupSavestates() {
	QFrame* savestatesWidget = new QFrame;
	addSection(savestatesWidget, tr("Savestates"), mTopRow);
}

void MainWindow::setupLog() {
	mLogWidget = new LogWidget;
	addSection(mLogWidget, tr("Log"), mTopRow);
	mLogWidget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
}

void MainWindow::setupInputDisplay() {
	QFrame* inputDisplayWidget = new QFrame;
	addSection(inputDisplayWidget, tr("Input display"), mBottomRow);
}

void MainWindow::setupGameInfo() {
	QTableWidget* gameInfoWidget = new QTableWidget;
	addSection(gameInfoWidget, tr("Game info"), mBottomRow);
	gameInfoWidget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
	gameInfoWidget->setColumnCount(2);
	gameInfoWidget->horizontalHeader()->setStretchLastSection(true);
	gameInfoWidget->verticalHeader()->hide();
	gameInfoWidget->horizontalHeader()->hide();
	gameInfoWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);

	// QFormLayout* form = new QFormLayout(gameInfoWidget);
	// gameInfoWidget->setLayout(form);

	auto addRow = [&gameInfoWidget](const QString& name, QTableWidgetItem* item) {
		item = new QTableWidgetItem;
		s32 rowIdx = gameInfoWidget->rowCount();
		gameInfoWidget->insertRow(rowIdx);
		gameInfoWidget->setItem(rowIdx, 0, new QTableWidgetItem(name));
		gameInfoWidget->setItem(rowIdx, 1, item);
	};

	addRow(tr("Stage name"), mGameInfo.stageName);
	addRow(tr("Player position"), mGameInfo.playerPos);
	QVector3D playerPos = { 0.0f, 0.0f, 0.0f };
	vec3ToString(playerPos);
}
