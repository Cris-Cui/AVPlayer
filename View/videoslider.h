#ifndef VIDEOSLIDER_H
#define VIDEOSLIDER_H

#include <QSlider>
#include <QStyle>
#include <QMouseEvent>

class VideoSlider : public  QSlider
{
    Q_OBJECT
public:
    explicit VideoSlider(QWidget *parent = 0);
    ~VideoSlider();
signals:
    void SIG_valueChanged(int);
protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // VIDEOSLIDER_H
