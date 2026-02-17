#include <QApplication>

#include "MainWindow.h"
#include "types.h"

s32 main(s32 argc, char* argv[]) {
	QApplication application(argc, argv);

	MainWindow window;
	window.setWindowTitle("Calypso Server");
	window.setWindowFlags(Qt::Dialog);
	window.show();

	return application.exec();
}
