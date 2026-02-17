#include <QApplication>

#include <hk/types.h>

#include "MainWindow.h"

s32 main(s32 argc, char* argv[]) {
	QApplication application(argc, argv);

	MainWindow window;
	window.setWindowTitle("Calypso Server");
	window.setWindowFlags(Qt::Dialog);
	window.show();

	return application.exec();
}
