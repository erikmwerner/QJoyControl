#ifndef INPUTMAPPANEL_H
#define INPUTMAPPANEL_H

#include <QWidget>
#include "inputmapwidget.h"

class InputMapWidget;

namespace Ui {
class InputMapPanel;
}

class InputMapPanel : public QWidget
{
    Q_OBJECT

public:
    explicit InputMapPanel(QWidget *parent = nullptr);
    ~InputMapPanel();
    InputMapWidget *addMapper(InputMapWidget* mapper);
    void setName(QString name);

private:
    Ui::InputMapPanel *ui;
};

#endif // INPUTMAPPANEL_H
