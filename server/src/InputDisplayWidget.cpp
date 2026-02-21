#include "InputDisplayWidget.h"

#include <QColor>
#include <QPainter>
#include <QPainterStateGuard>

#include <hk/types.h>

InputDisplayWidget::InputDisplayWidget(QWidget* parent) : QFrame(parent) {}

void InputDisplayWidget::setInputs(hk::util::Vector2i leftStick, hk::util::Vector2i rightStick, u64 buttons) {
	inputs.leftStick = { (leftStick.x + 32768) / 32768.0f - 1.0f, (leftStick.y + 32768) / 32768.0f - 1.0f };
	inputs.rightStick = { (rightStick.x + 32768) / 32768.0f - 1.0f, (rightStick.y + 32768) / 32768.0f - 1.0f };
	inputs.buttons.raw = buttons;
}

void InputDisplayWidget::paintEvent(QPaintEvent* event) {
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);

	painter.translate(width() / 2.0f, height() / 2.0f);

	f32 outerRatio = f32(width()) / f32(height());
	f32 innerRatio = cInnerSize.x() / cInnerSize.y();

	QVector2D innerSize;
	if (outerRatio > innerRatio) {
		innerSize.setY(height());
		innerSize.setX(innerSize.y() * innerRatio);
	} else {
		innerSize.setX(width());
		innerSize.setY(innerSize.x() / innerRatio);
	}

	painter.scale(innerSize.x() / cInnerSize.x(), innerSize.y() / cInnerSize.y());

	{
		QPainterStateGuard guard(&painter);

		painter.drawEllipse(QPoint { -120, 15 }, 30, 30);
		painter.drawEllipse(QPoint { 120, 15 }, 30, 30);

		painter.setPen(Qt::NoPen);

		// left stick
		painter.setBrush(inputs.buttons.bits.leftStickPress ? onColor : offColor);
		QVector2D leftStickPos = QVector2D(-120, 15) + 20.0f * inputs.leftStick;
		painter.drawEllipse(QPoint(leftStickPos.x(), leftStickPos.y()), 20, 20);

		// right stick
		painter.setBrush(inputs.buttons.bits.rightStickPress ? onColor : offColor);
		QVector2D rightStickPos = QVector2D(120, 15) + 20.0f * inputs.rightStick;
		painter.drawEllipse(QPoint(rightStickPos.x(), rightStickPos.y()), 20, 20);

		// d-pad
		painter.setBrush(inputs.buttons.bits.dpadLeft ? onColor : offColor);
		painter.drawEllipse(QPoint { -60, 15 }, 10, 10);
		painter.setBrush(inputs.buttons.bits.dpadDown ? onColor : offColor);
		painter.drawEllipse(QPoint { -40, 35 }, 10, 10);
		painter.setBrush(inputs.buttons.bits.dpadUp ? onColor : offColor);
		painter.drawEllipse(QPoint { -40, -5 }, 10, 10);
		painter.setBrush(inputs.buttons.bits.dpadRight ? onColor : offColor);
		painter.drawEllipse(QPoint { -20, 15 }, 10, 10);

		// abxy
		painter.setBrush(inputs.buttons.bits.a ? onColor : offColor);
		painter.drawEllipse(QPoint { 60, 15 }, 10, 10);
		painter.setBrush(inputs.buttons.bits.b ? onColor : offColor);
		painter.drawEllipse(QPoint { 40, 35 }, 10, 10);
		painter.setBrush(inputs.buttons.bits.x ? onColor : offColor);
		painter.drawEllipse(QPoint { 40, -5 }, 10, 10);
		painter.setBrush(inputs.buttons.bits.y ? onColor : offColor);
		painter.drawEllipse(QPoint { 20, 15 }, 10, 10);

		// zl/l
		painter.setBrush(inputs.buttons.bits.zl ? onColor : offColor);
		painter.drawRoundedRect(-160, -60, 60, 20, 10, 10);
		painter.setBrush(inputs.buttons.bits.l ? onColor : offColor);
		painter.drawRoundedRect(-85, -60, 40, 20, 10, 10);

		// zr/r
		painter.setBrush(inputs.buttons.bits.zr ? onColor : offColor);
		painter.drawRoundedRect(100, -60, 60, 20, 10, 10);
		painter.setBrush(inputs.buttons.bits.r ? onColor : offColor);
		painter.drawRoundedRect(45, -60, 40, 20, 10, 10);

		// plus/minus
		painter.setBrush(inputs.buttons.bits.plus ? onColor : offColor);
		painter.drawEllipse(QPoint { -20, -35 }, 7, 7);
		painter.setBrush(inputs.buttons.bits.minus ? onColor : offColor);
		painter.drawEllipse(QPoint { 20, -35 }, 7, 7);
	}
}
