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

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
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
    QHBoxLayout* topRow    = new QHBoxLayout;
    QHBoxLayout* bottomRow = new QHBoxLayout;
    vboxLayout->addLayout(topRow);
    vboxLayout->addLayout(bottomRow);

    {
        QVBoxLayout* vboxLayout = new QVBoxLayout;
        topRow->addLayout(vboxLayout);

        QLabel* label = new QLabel(tr("Controls"));
        label->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
        vboxLayout->addWidget(label);

        QFrame* controlsWidget = new QFrame;
        controlsWidget->setFrameShape(QFrame::StyledPanel);
        vboxLayout->addWidget(controlsWidget);
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

    {
        QVBoxLayout* vboxLayout = new QVBoxLayout;
        topRow->addLayout(vboxLayout);

        QLabel* label = new QLabel(tr("Savestates"));
        label->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
        vboxLayout->addWidget(label);

        QFrame* savestatesWidget = new QFrame;
        savestatesWidget->setFrameShape(QFrame::StyledPanel);
        vboxLayout->addWidget(savestatesWidget);
    }

    {
        QVBoxLayout* vboxLayout = new QVBoxLayout;
        topRow->addLayout(vboxLayout);

        QLabel* label = new QLabel(tr("Log"));
        label->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
        vboxLayout->addWidget(label);

        mLogWidget = new LogWidget;
        mLogWidget->setFrameShape(QFrame::StyledPanel);
        mLogWidget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
        vboxLayout->addWidget(mLogWidget);
        
        for (int i = 0; i < 100; i++)
            mLogWidget->appendLine("meow");
    }
    
    {
        QVBoxLayout* vboxLayout = new QVBoxLayout;
        bottomRow->addLayout(vboxLayout);

        QLabel* label = new QLabel(tr("Input display"));
        label->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
        vboxLayout->addWidget(label);

        QFrame* inputDisplayWidget = new QFrame;
        inputDisplayWidget->setFrameShape(QFrame::StyledPanel);
        vboxLayout->addWidget(inputDisplayWidget);
    }

    {
        QVBoxLayout* vboxLayout = new QVBoxLayout;
        bottomRow->addLayout(vboxLayout);

        QLabel* label = new QLabel("Game info");
        label->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
        vboxLayout->addWidget(label);

        QFrame* gameInfoWidget = new QFrame;
        gameInfoWidget->setFrameShape(QFrame::StyledPanel);
        vboxLayout->addWidget(gameInfoWidget);
    }

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
