#include "inputmapwidget.h"
#include "ui_inputmapwidget.h"
#include <QTimer>
#include <QKeyEvent>
#include "mainwindow.h"
#include "eventhandler.h"

#include <QSettings>
#include <QDebug>

InputMapWidget::InputMapWidget(QString button_name, int button_mask, int key_code, EventHandler* handler, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InputMapWidget),
    _handler(handler),
    _grab_timer(new QTimer(this)),
    _button_mask(button_mask),
    _button_name(button_name),
    _key_code(key_code),
    _wait_key(false)
{
    ui->setupUi(this);
    QSettings settings;
    if(settings.contains("key-map/" + QString::number(_button_mask)) ) {
        int code = settings.value("key-map/" + QString::number(_button_mask)).toInt();
        _key_code = code;
    }

    connect(_grab_timer,SIGNAL(timeout()),this,SLOT(onGrabTimerTimeout()));
    ui->labelButton->setText(button_name);
    displayKeyCodeString();
    handler->addMapping(_button_mask, _key_code);
}

InputMapWidget::~InputMapWidget()
{
    QSettings settings;//("werner", "QJoyControl");
    settings.beginGroup("key-map");
    settings.setValue(QString::number(_button_mask), _key_code);
    settings.endGroup();

    delete ui;
}

void InputMapWidget::displayKeyCodeString()
{
    Qt::Key key = (Qt::Key)_key_code;
    ui->labelKeyCode->setText(QKeySequence(key).toString());
}

void InputMapWidget::on_toolButtonSet_clicked()
{
    // get the next keypress
    _wait_key = true;
    grab_timer_count = 0;
    ui->toolButtonSet->setText(QString::number(10 - grab_timer_count) + "...");
    ui->toolButtonSet->setEnabled(false);
    grabKeyboard();
    _grab_timer->start(1000);
}

void InputMapWidget::keyPressEvent(QKeyEvent *event)
{
    if(_wait_key){
        _key_code = event->key();
        _handler->addMapping(_button_mask, _key_code);
        displayKeyCodeString();
        _grab_timer->stop();
        onGrabKeyFinished();
    }
    else {
        QWidget::keyPressEvent(event);
    }
}

void InputMapWidget::onGrabTimerTimeout()
{
    grab_timer_count++;
    ui->toolButtonSet->setText(QString::number(10 - grab_timer_count) + "...");
    if(grab_timer_count > 10)
    {
        onGrabKeyFinished();
    }
}

void InputMapWidget::onGrabKeyFinished()
{
    _wait_key = false;
    // release the keyboard
    releaseKeyboard();
    _grab_timer->stop();
    ui->toolButtonSet->setText("Set");
    ui->toolButtonSet->setEnabled(true);
}

void InputMapWidget::setClickMap(int code)
{
    if(code == 0) { // no click
        setEnabled(true);
        _handler->addMapping(_button_mask, _key_code);
    }
    else if(code == 1) {  // right click
        setEnabled(false);
        _handler->mapToMouseButton(_button_mask, 1);
        //_handler->addMapping(_button_mask, kCGEventRightMouseDown);
    }
    else if (code == 2) { // left click
        setEnabled(false);
        _handler->mapToMouseButton(_button_mask, 2);
       // _handler->addMapping(_button_mask, kCGEventLeftMouseDown);
    }
}
