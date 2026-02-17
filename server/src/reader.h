#pragma once

#include <QDataStream>
#include <QString>

#include <hk/types.h>

namespace reader {

hk::Result checkSignature(QDataStream& stream, size offset, const QString& expected);

}
