#pragma once

#include <QPlainTextEdit>

class LogWidget : public QPlainTextEdit {
	Q_OBJECT

public:
	LogWidget(QWidget* parent = nullptr);
	~LogWidget() override = default;

	void appendLine(const QString& text);

#ifdef __GNUC__
	[[gnu::format(printf, 2, 3)]]
#endif
	void log(const char* fmt, ...) {
		va_list args;
		va_start(args, fmt);

		QString out = QString::vasprintf(fmt, args);

		appendLine(out);

		va_end(args);
	}
};
