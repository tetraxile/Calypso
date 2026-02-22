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
#include <QScrollBar>
#include <QSizePolicy>
#include <QTableWidget>
#include <QVector3D>

#include <hk/ValueOrResult.h>
#include <hk/diag/diag.h>
#include <hk/types.h>

#include "results.h"
#include "util.h"

void MainWindow::openFileButton() {
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select a script"), "", tr("STAS scripts (*.stas)"));
	if (fileName.isEmpty()) return;

	openFile(fileName, true);
}

void MainWindow::openFileRecent() {
	QString fileName = mRecentScripts->currentText();

	openFile(fileName, false);
}

void MainWindow::openFile(const QString& fileName, bool isSetRecent) {
	mControls.disable();

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		mLogWidget->log("could not open file: %s", qPrintable(fileName));
		return;
	}

	mLogWidget->log("opening file: %s", qPrintable(fileName));
	if (mScript) {
		delete mScript;
		mScript = nullptr;
	}
	mScript = new ScriptSTAS(file, *mLogWidget);

	if (isSetRecent) {
		mRecentScripts->insertItem(0, fileName);
		mRecentScripts->setCurrentIndex(0);
	}

	if (parseScript().failed()) {
		clearScript();
		return;
	}

	mControls.enable();
}

hk::Result MainWindow::parseScript() {
	hk::Result r = mScript->readHeader();
	if (r == hk::ResultFailed()) {
		return hk::ResultFailed();
	} else if (r == ResultEOFReached()) {
		mLogWidget->log("unexpected EOF while parsing script!");
		return hk::ResultFailed();
	} else if (r.failed()) {
		mLogWidget->log("error parsing script header: %04d-%04d", r.getModule(), r.getDescription());
		return r;
	}

	r = mScript->verify();
	if (r == hk::ResultFailed()) {
		return hk::ResultFailed();
	} else if (r == ResultEOFReached()) {
		mLogWidget->log("unexpected EOF while parsing script!");
		return hk::ResultFailed();
	} else if (r.failed()) {
		mLogWidget->log("error parsing script: %04d-%04d", r.getModule(), r.getDescription());
		return r;
	}

	r = mScript->readMetadata();
	if (r == hk::ResultFailed()) {
		return hk::ResultFailed();
	} else if (r.failed()) {
		mLogWidget->log("error parsing script metadata: %04d-%04d", r.getModule(), r.getDescription());
		return r;
	}

	mScriptInfo.name->setText(mScript->mInfo.name);
	mScriptInfo.author->setText(mScript->mInfo.author);
	mScriptInfo.commandCount->setText(QString::number(mScript->mInfo.frameCount));

	mScript->close();

	return hk::ResultSuccess();
}

void MainWindow::clearScript() {
	mScriptInfo.name->clear();
	mScriptInfo.author->clear();
	mScriptInfo.commandCount->clear();
	delete mScript;
	mScript = nullptr;
	mControls.disable();
}

void MainWindow::runScript() {
	HK_ASSERT(mScript != nullptr);
	mLogWidget->log("running script...");

	mServer->log("test");
}

void MainWindow::stopScript() {
	HK_ASSERT(mScript != nullptr);
	mLogWidget->log("stopped script");
}

void MainWindow::pauseGame() {
	HK_ASSERT(mScript != nullptr);
	mLogWidget->log("pausing game");
}

void MainWindow::frameAdvance() {
	HK_ASSERT(mScript != nullptr);
	mLogWidget->log("advancing to next frame");

	getNextFrame();
}

void MainWindow::getNextFrame() {
	hk::ValueOrResult<ScriptSTAS::Packet&> packetR = mScript->getNextFrame();
	if (packetR == ResultEndOfScriptReached()) {
		clearScript();
		return;
	} else if (!packetR.hasValue()) {
		hk::Result r = packetR;
		mLogWidget->log("error getting frame: %04d-%04d", r.getModule(), r.getDescription());
		clearScript();
		return;
	}

	ScriptSTAS::Packet& packet = packetR;

	auto& player1 = packet.frame.player1;
	mInputDisplayWidget->setInputs(player1.leftStick, player1.rightStick, player1.buttons);

	mInputDisplayWidget->update();
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
	setupUi();

	mServer = new Server(*mLogWidget);
}

void MainWindow::closeEvent(QCloseEvent* event) {
	mServer->close();

	if (mScript) mScript->close();

	event->accept();
}

MainWindow::~MainWindow() {}

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

		mControls.playButton = new QPushButton(tr("Run script"));
		mControls.playButton->setDisabled(true);
		connect(mControls.playButton, &QPushButton::clicked, this, &MainWindow::runScript);
		row->addWidget(mControls.playButton);

		mControls.stopButton = new QPushButton(tr("Stop script"));
		mControls.stopButton->setDisabled(true);
		connect(mControls.stopButton, &QPushButton::clicked, this, &MainWindow::stopScript);
		row->addWidget(mControls.stopButton);
	}
	{
		QHBoxLayout* row = new QHBoxLayout;
		controlsLayout->addLayout(row);

		mControls.pauseButton = new QPushButton(tr("Toggle game pause"));
		mControls.pauseButton->setDisabled(true);
		connect(mControls.pauseButton, &QPushButton::clicked, this, &MainWindow::pauseGame);
		row->addWidget(mControls.pauseButton);

		mControls.frameAdvanceButton = new QPushButton(tr("Frame advance"));
		mControls.frameAdvanceButton->setDisabled(true);
		connect(mControls.frameAdvanceButton, &QPushButton::clicked, this, &MainWindow::frameAdvance);
		row->addWidget(mControls.frameAdvanceButton);
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
	mInputDisplayWidget = new InputDisplayWidget;
	addSection(mInputDisplayWidget, tr("Input display"), mBottomRow);
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
