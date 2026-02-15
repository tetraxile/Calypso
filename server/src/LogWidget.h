#pragma once

#include <QPlainTextEdit>

class LogWidget : public QPlainTextEdit {
	Q_OBJECT

public:
	LogWidget(QWidget* parent = nullptr);
	~LogWidget() override = default;

	void appendLine(const QString& text);
};
