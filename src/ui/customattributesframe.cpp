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

    this->table = ui->tableWidget;

    // Connect the "Add" button
    connect(ui->button_Add, &QPushButton::clicked, [this]() {
        CustomAttributesDialog dialog(this->table);
        dialog.exec();
    });

    // Connect the "Delete" button
    connect(ui->button_Delete, &QPushButton::clicked, [this]() {
        this->table->deleteSelectedAttributes();
    });
}
