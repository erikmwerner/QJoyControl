#include "statuswidget.h"
#include "ui_statuswidget.h"
#include <QLabel>
#include <qdebug.h>

StatusWidget::StatusWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StatusWidget),
    _battery_pixmap(QPixmap())
{
    ui->setupUi(this);
}

StatusWidget::~StatusWidget()
{
    delete ui;
}

void StatusWidget::onBatteryData(int batt, QString text){
    ui->labelBatteryText->setText(text);

    // Update Battery icon from input report value.
    switch (batt) {
    case 0:
        _battery_pixmap.load(":/img/batt_0.png");
        break;
    case 1:
        _battery_pixmap.load(":/img/batt_0_chr.png");
        break;
    case 2:
        _battery_pixmap.load(":/img/batt_25.png");
        break;
    case 3:
        _battery_pixmap.load(":/img/batt_25_cr.png");
        break;
    case 4:
        _battery_pixmap.load(":/img/batt_50.png");
        break;
    case 5:
        _battery_pixmap.load(":/img/batt_50_cr.png");
        break;
    case 6:
        _battery_pixmap.load(":/img/batt_75.png");
        break;
    case 7:
        _battery_pixmap.load(":/img/batt_75_cr.png");
        break;
    case 8:
        _battery_pixmap.load(":/img/batt_100.png");
        break;
    case 9:
        _battery_pixmap.load(":/img/batt_100_cr.png");
        break;
    default:
        _battery_pixmap.load(":/img/batt_0.png");
    }

    ui->labelBatteryGraphic->setPixmap(_battery_pixmap.scaled(24,9));
}

void StatusWidget::onTemperatureData(float temp){
    QString text_string("Temp: ");
    text_string += QString::number(temp, 'f', 1);
    text_string +=("ºC ");
    ui->labelTempText->setText(text_string);
}
