#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

#include <hk/types.h>

#include "InputDisplayWidget.h"
#include "LogWidget.h"
#include "ScriptSTAS.h"

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;

	void setupUi();
	void retranslateUi();

private slots:
	void newConnection();
	void disconnected();
	void receiveTCPData();
	void receiveUDPData();
	void openFileButton();
	void openFileRecent();
	void runScript();
	void stopScript();
	void pauseGame();
	void frameAdvance();

private:
	void closeEvent(QCloseEvent* event) override;

	void setupControls();
	void setupSavestates();
	void setupLog();
	void setupInputDisplay();
	void setupGameInfo();
	void addSection(QFrame* section, const QString& name, QHBoxLayout* row);

	void openFile(const QString& fileName, bool isSetRecent);
	hk::Result parseScript();
	void clearScript();
	void getNextFrame();

	static const s32 RECENT_SCRIPTS_NUM = 10;
	static const s32 PORT = 8171;

	LogWidget* mLogWidget;
	QHBoxLayout* mBottomRow;
	QHBoxLayout* mTopRow;
	QMenuBar* mMenuBar;
	QStatusBar* mStatusBar;
	QComboBox* mRecentScripts;

	InputDisplayWidget* mInputDisplayWidget;

	struct {
		QPushButton* playButton;
		QPushButton* stopButton;
		QPushButton* pauseButton;
		QPushButton* frameAdvanceButton;

		void enable() {
			playButton->setEnabled(true);
			stopButton->setEnabled(true);
			pauseButton->setEnabled(true);
			frameAdvanceButton->setEnabled(true);
		}

		void disable() {
			playButton->setEnabled(false);
			stopButton->setEnabled(false);
			pauseButton->setEnabled(false);
			frameAdvanceButton->setEnabled(false);
		}
	} mControls;

	struct {
		QLineEdit* name;
		QLineEdit* author;
		QLineEdit* commandCount;
	} mScriptInfo;

	struct {
		QTableWidgetItem* stageName;
		QTableWidgetItem* playerPos;
	} mGameInfo;

	QTcpServer* mServer = nullptr;
	QTcpSocket* mTCPSocket = nullptr;
	QUdpSocket* mUDPSocket = nullptr;
	ScriptSTAS* mScript = nullptr;
};
