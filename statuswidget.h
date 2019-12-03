#ifndef STATUSWIDGET_H
#define STATUSWIDGET_H

#include <QWidget>

namespace Ui {
class StatusWidget;
}

class StatusWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatusWidget(QWidget *parent = nullptr);
    ~StatusWidget();

public slots:
    void onBatteryData(int batt, QString text);
    void onTemperatureData(float temp);
private:
    Ui::StatusWidget *ui;
    QPixmap _battery_pixmap;
};

#endif // STATUSWIDGET_H
