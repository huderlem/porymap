#include "preferenceeditor.h"
#include "ui_preferenceeditor.h"
#include "config.h"

#include <QAbstractButton>


PreferenceEditor::PreferenceEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PreferenceEditor)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &PreferenceEditor::dialogButtonClicked);
    populateFields();
}

PreferenceEditor::~PreferenceEditor()
{
    delete ui;
}

void PreferenceEditor::populateFields() {
    ui->lineEdit_TextEditor->setText(porymapConfig.getTextEditorCommand());
}

void PreferenceEditor::saveFields() {
    porymapConfig.setTextEditorCommand(ui->lineEdit_TextEditor->text());
}

void PreferenceEditor::dialogButtonClicked(QAbstractButton *button) {
    auto buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::AcceptRole) {
        saveFields();
        close();
    } else if (buttonRole == QDialogButtonBox::ApplyRole) {
        saveFields();
    } else if (buttonRole == QDialogButtonBox::RejectRole) {
        close();
    }
}
