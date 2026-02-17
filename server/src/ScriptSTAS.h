#pragma once

#include <QFile>

#include <hk/Result.h>
#include <hk/types.h>

struct ScriptSTAS {
	ScriptSTAS(QFile& file);
	hk::Result readHeader();
	void close();

	QFile& file;
	QString name;
	QString author;
};
