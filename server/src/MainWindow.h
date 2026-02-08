#pragma once

#include <QMainWindow>

#include "LogWidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void setupUi();
    void retranslateUi();

    void log(const QString& text) { mLogWidget->appendLine(text); }

private:
    LogWidget* mLogWidget = nullptr;

    QMenuBar *mMenuBar;
    QMenu *menuMeow;
    QStatusBar *mStatusBar;
};