#pragma once

#include <QFrame>
#include <QVector2D>

#include <hk/types.h>
#include <hk/util/Math.h>

class InputDisplayWidget : public QFrame {
	Q_OBJECT

public:
	struct Inputs {
		QVector2D leftStick;
		QVector2D rightStick;

		union {
			struct {
				bool a : 1;
				bool b : 1;
				bool x : 1;
				bool y : 1;
				bool leftStickPress : 1;
				bool rightStickPress : 1;
				bool l : 1;
				bool r : 1;
				bool zl : 1;
				bool zr : 1;
				bool plus : 1;
				bool minus : 1;
				bool dpadLeft : 1;
				bool dpadUp : 1;
				bool dpadRight : 1;
				bool dpadDown : 1;
			} bits;

			u64 raw;
		} buttons;
	};

	InputDisplayWidget(QWidget* parent = nullptr);

	~InputDisplayWidget() override = default;

	void setInputs(hk::util::Vector2i leftStick, hk::util::Vector2i rightStick, u64 buttons);

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	constexpr static QVector2D cInnerSize = { 340, 140 };

	Inputs inputs;
	QColor onColor = QColor::fromHslF(0, 0, 0.3);
	QColor offColor = QColor::fromHslF(0, 0, 0.7);
};
