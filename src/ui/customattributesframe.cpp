#include "customattributesframe.h"
#include "ui_customattributesframe.h"
#include "customattributesdialog.h"
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

CustomAttributesFrame::CustomAttributesFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::CustomAttributesFrame)
{
    ui->setupUi(this);

    connect(ui->button_Add, &QPushButton::clicked, this, &CustomAttributesFrame::addAttribute);
    connect(ui->button_Delete, &QPushButton::clicked, this, &CustomAttributesFrame::deleteAttribute);
    connect(ui->tableWidget, &CustomAttributesTable::itemSelectionChanged, this, &CustomAttributesFrame::updateDeleteButton);
    connect(ui->tableWidget, &CustomAttributesTable::edited, this, &CustomAttributesFrame::updateDeleteButton);
}

CustomAttributesFrame::~CustomAttributesFrame() {
    delete ui;
}

CustomAttributesTable* CustomAttributesFrame::table() const {
    return ui->tableWidget;
}

void CustomAttributesFrame::addAttribute() {
    auto dialog = new CustomAttributesDialog(ui->tableWidget);
    dialog->open();
}

void CustomAttributesFrame::deleteAttribute() {
    ui->tableWidget->deleteSelectedAttributes();
}

void CustomAttributesFrame::updateDeleteButton() {
    ui->button_Delete->setDisabled(ui->tableWidget->isSelectionEmpty());
}
