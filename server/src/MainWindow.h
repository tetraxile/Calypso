#pragma once

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QTableWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>

#include "LogWidget.h"
#include "ScriptSTAS.h"
#include "types.h"

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;

	void setupUi();
	void retranslateUi();

#ifdef __GNUC__
	[[gnu::format(printf, 2, 3)]]
#endif
	void log(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);

		QString out = QString::vasprintf(fmt, args);

		mLogWidget->appendLine(out);

		va_end(args);
	}

private slots:
	void newConnection();
	void disconnected();
	void receiveTCPData();
	void receiveUDPData();
	void openFileButton();
	void openFileRecent();

private:
	void closeEvent(QCloseEvent* event) override;

	void setupControls();
	void setupSavestates();
	void setupLog();
	void setupInputDisplay();
	void setupGameInfo();
	void addSection(QFrame* section, const QString& name, QHBoxLayout* row);

	static const s32 RECENT_SCRIPTS_NUM = 10;
	static const s32 PORT = 8171;

	LogWidget* mLogWidget;
	QHBoxLayout* mBottomRow;
	QHBoxLayout* mTopRow;
	QMenuBar* mMenuBar;
	QStatusBar* mStatusBar;
	QComboBox* mRecentScripts;

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
