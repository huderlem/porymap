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

    QStringList themes = { "default" };
    QRegularExpression re(":/themes/([A-z0-9_-]+).qss");
    QDirIterator it(":/themes", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString themeName = re.match(it.next()).captured(1);
        themes.append(themeName);
    }
    themeSelector->addItems(themes);
    themeSelector->setCurrentText(porymapConfig.getTheme());
}

void PreferenceEditor::saveFields() {
    porymapConfig.setTextEditorCommand(ui->lineEdit_TextEditor->text());

    if (themeSelector->currentText() != porymapConfig.getTheme()) {
        const auto theme = themeSelector->currentText();
        porymapConfig.setTheme(theme);
        emit themeChanged(theme);
    }

    emit preferencesSaved();
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
