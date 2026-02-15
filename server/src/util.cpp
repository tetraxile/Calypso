#include "util.h"

QString vec3ToString(const std::optional<QVector3D>& vec) {
    if (vec->isNull()) {
        return "-";
    }

    return QString("(%1, %2, %3)").arg(QString::number(vec->x()), QString::number(vec->y()), QString::number(vec->z()));
}
