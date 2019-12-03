#ifndef INPUTMAPWIDGET_H
#define INPUTMAPWIDGET_H

#include <QWidget>

class EventHandler;

namespace Ui {
class InputMapWidget;
}

class InputMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit InputMapWidget(QString button_name, int button_mask, int key_code, EventHandler* handler, QWidget *parent = nullptr);
    ~InputMapWidget();

    void setClickMap(int code);
protected:
    void keyPressEvent(QKeyEvent *event);

private slots:
    void on_toolButtonSet_clicked();
    void onGrabTimerTimeout();
    void onGrabKeyFinished();

private:
    void displayKeyCodeString();

    Ui::InputMapWidget *ui;
    EventHandler* _handler = nullptr;
    QTimer* _grab_timer = nullptr;
    int grab_timer_count = 0;
    int _button_mask = 0;
    QString _button_name;
    int _key_code;
    bool _wait_key;;
};

#endif // INPUTMAPWIDGET_H
