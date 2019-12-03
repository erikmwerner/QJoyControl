#include "inputmappanel.h"
#include "ui_inputmappanel.h"

InputMapPanel::InputMapPanel(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::InputMapPanel)
{
    ui->setupUi(this);
}

InputMapPanel::~InputMapPanel()
{
    delete ui;
}

InputMapWidget* InputMapPanel::addMapper(InputMapWidget* mapper)
{
    // row, col, row span, col span
    ui->gridLayout->addWidget(mapper, ui->gridLayout->rowCount(), 0, 1, 3);
    return mapper;
}

void InputMapPanel::setName(QString name)
{
    ui->groupBox->setTitle(name);
}
