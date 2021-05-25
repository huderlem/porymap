#include "preferenceeditor.h"
#include "ui_preferenceeditor.h"
#include "config.h"
#include "noscrollcombobox.h"

#include <QAbstractButton>
#include <QRegularExpression>
#include <QDirIterator>
#include <QFormLayout>


PreferenceEditor::PreferenceEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PreferenceEditor),
    themeSelector(nullptr)
{
    ui->setupUi(this);
    auto *formLayout = new QFormLayout(ui->groupBox_Themes);
    themeSelector = new NoScrollComboBox(ui->groupBox_Themes);
    formLayout->addRow("Themes", themeSelector);
    ui->spinBox_AutoSaveDelay->setRange(0, INT_MAX);
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
    QStringList themes("default");
    QRegularExpression re(":/themes/([A-z0-9_-]+).qss");
    QDirIterator it(":/themes", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString themeName = re.match(it.next()).captured(1);
        themes.append(themeName);
    }
    themeSelector->addItems(themes);
    themeSelector->setCurrentText(porymapConfig.getTheme());

    ui->spinBox_AutoSaveDelay->setValue(porymapConfig.getAutoSaveDelay());
    ui->checkBox_AutoSaveOnMapChange->setChecked(porymapConfig.getAutoSaveOnMapChange());

    ui->lineEdit_TextEditorOpenFolder->setText(porymapConfig.getTextEditorOpenFolder());
    ui->lineEdit_TextEditorGotoLine->setText(porymapConfig.getTextEditorGotoLine());
}

void PreferenceEditor::saveFields() {
    if (themeSelector->currentText() != porymapConfig.getTheme()) {
        const auto theme = themeSelector->currentText();
        porymapConfig.setTheme(theme);
        emit themeChanged(theme);
    }

    porymapConfig.setAutoSaveDelay(ui->spinBox_AutoSaveDelay->value());
    porymapConfig.setAutoSaveOnMapChange(ui->checkBox_AutoSaveOnMapChange->isChecked());

    porymapConfig.setTextEditorOpenFolder(ui->lineEdit_TextEditorOpenFolder->text());
    porymapConfig.setTextEditorGotoLine(ui->lineEdit_TextEditorGotoLine->text());

    emit preferencesSaved();
}

void PreferenceEditor::dialogButtonClicked(QAbstractButton *button) {
    const auto buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::AcceptRole) {
        saveFields();
        close();
    } else if (buttonRole == QDialogButtonBox::ApplyRole) {
        saveFields();
    } else if (buttonRole == QDialogButtonBox::RejectRole) {
        close();
    }
}
