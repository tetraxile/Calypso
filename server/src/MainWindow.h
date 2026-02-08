#pragma once

#include <QHBoxLayout>
#include <QMainWindow>
#include <QTcpSocket>
#include <QTcpServer>

#include "LogWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
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
    void readClient();

private:
    void setupControls();
	void setupSavestates();
	void setupLog();
	void setupInputDisplay();
	void setupGameInfo();
    void addSection(QFrame* section, const QString& name, QHBoxLayout* row);

    LogWidget* mLogWidget;
    QHBoxLayout* mBottomRow;
    QHBoxLayout* mTopRow;
    QMenuBar *mMenuBar;
    QMenu *menuMeow;
    QStatusBar *mStatusBar;

    QTcpServer* mServer = nullptr;
    QTcpSocket* mClientSocket = nullptr;
};