#include "videoslider.h"

VideoSlider::VideoSlider(QWidget *parent)
    : QSlider(parent) {

}

VideoSlider::~VideoSlider()
{

}

void VideoSlider::mousePressEvent(QMouseEvent *event)
{
    int value = QStyle::sliderValueFromPosition(
                minimum(), maximum(), event->pos().x(), width());
    setValue(value);
    emit SIG_valueChanged(value);
}
