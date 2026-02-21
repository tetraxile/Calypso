#include "LogWidget.h"

#include <QScrollBar>

LogWidget::LogWidget(QWidget* parent) : QPlainTextEdit(parent) {
	setReadOnly(true);
	setMaximumBlockCount(5000);
}

void LogWidget::appendLine(const QString& text) {
	appendPlainText(text);
	verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}
