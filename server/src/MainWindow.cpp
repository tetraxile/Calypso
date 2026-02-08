#include "MainWindow.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QFormLayout>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSizePolicy>
#include <QStatusBar>
#include <QTableWidget>


void MainWindow::newConnection() {
    mClientSocket = mServer->nextPendingConnection();
    
    {
        bool isV4 = false;
        QHostAddress addrIPv6 = mClientSocket->peerAddress();
        QHostAddress addrIPv4 = QHostAddress(addrIPv6.toIPv4Address(&isV4));
        QString addrStr = isV4 ? addrIPv4.toString() : addrIPv6.toString();
        quint16 port = mClientSocket->peerPort();
        log("connection received from %s:%d", qPrintable(addrStr), port);
    }

    connect(mClientSocket, &QTcpSocket::disconnected, mClientSocket, &QTcpSocket::deleteLater);
    connect(mClientSocket, &QTcpSocket::disconnected, this, &MainWindow::disconnected);
    connect(mClientSocket, &QTcpSocket::readyRead, this, &MainWindow::readClient);
}

void MainWindow::disconnected() {
    log("client disconnected");
}

void MainWindow::readClient() {
    QTextStream in(mClientSocket);
    log("[switch] %s", qPrintable(in.readAll()));
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();

    mServer = new QTcpServer(this);
    mServer->listen(QHostAddress::Any, 8171);
    log("listening for connection on port %d...", 8171);
    connect(mServer, &QTcpServer::newConnection, this, &MainWindow::newConnection);
}

MainWindow::~MainWindow() {
	// delete mViewport;
}

void MainWindow::setupUi() {
    if (objectName().isEmpty())
        setObjectName("MainWindow");
    resize(1280, 720);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* vboxLayout = new QVBoxLayout(centralWidget);
    mTopRow    = new QHBoxLayout;
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
    menuMeow = new QMenu(mMenuBar);
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
    menuMeow->setTitle(QCoreApplication::translate("MainWindow", "Meow", nullptr));
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

		QComboBox* recentScripts = new QComboBox;
		recentScripts->addItem("");
		recentScripts->addItem("mreow");
		recentScripts->addItem("mreow");
		recentScripts->addItem("mreow");
		row->addWidget(recentScripts);

		QPushButton* openButton = new QPushButton(tr("Open"));
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

		QLineEdit* scriptNameWidget = new QLineEdit;
		scriptNameWidget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
		scriptNameWidget->setReadOnly(true);
		form->addRow(tr("Script name"), scriptNameWidget);

		QLineEdit* authorWidget = new QLineEdit;
		authorWidget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
		authorWidget->setReadOnly(true);
		authorWidget->setCursorPosition(0);
		form->addRow(tr("Author"), authorWidget);

		QLineEdit* commandCountWidget = new QLineEdit;
		commandCountWidget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
		commandCountWidget->setReadOnly(true);
		form->addRow(tr("Command count"), commandCountWidget);
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
	QFrame* gameInfoWidget = new QFrame;
	addSection(gameInfoWidget, tr("Game info"), mBottomRow);

	QFormLayout* form = new QFormLayout(gameInfoWidget);
	gameInfoWidget->setLayout(form);

	QLineEdit* stageName = new QLineEdit;
	stageName->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
	stageName->setReadOnly(true);
	form->addRow(tr("Stage name"), stageName);

	QLineEdit* playerPos = new QLineEdit;
	playerPos->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
	playerPos->setReadOnly(true);
	form->addRow(tr("Player position"), playerPos);
}
