#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
	QApplication application(argc, argv);

	MainWindow window;
	window.setWindowTitle("Calypso Server");
	window.setWindowFlags(Qt::Dialog);
	window.show();

	return application.exec();
}
