#include "preferenceeditor.h"
#include "ui_preferenceeditor.h"
#include "noscrollcombobox.h"
#include "message.h"

#include <QAbstractButton>
#include <QRegularExpression>
#include <QDirIterator>
#include <QFormLayout>
#include <QFontDialog>
#include <QToolTip>


PreferenceEditor::PreferenceEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PreferenceEditor),
    themeSelector(nullptr)
{
    ui->setupUi(this);
    auto *formLayout = new QFormLayout(ui->groupBox_Themes);
    themeSelector = new NoScrollComboBox(ui->groupBox_Themes);
    themeSelector->setEditable(false);
    themeSelector->setMinimumContentsLength(0);
    formLayout->addRow("Themes", themeSelector);
    setAttribute(Qt::WA_DeleteOnClose);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &PreferenceEditor::dialogButtonClicked);

    connect(ui->pushButton_CustomizeApplicationFont, &QPushButton::clicked, [this] {
        openFontDialog(&this->applicationFont);
    });
    connect(ui->pushButton_CustomizeMapListFont, &QPushButton::clicked, [this] {
        openFontDialog(&this->mapListFont);
    });
    connect(ui->pushButton_ResetApplicationFont, &QPushButton::clicked, [this] {
        this->applicationFont = QFont();
        QToolTip::showText(ui->pushButton_ResetApplicationFont->mapToGlobal(QPoint(0, 0)), "Font reset!");
    });
    connect(ui->pushButton_ResetMapListFont, &QPushButton::clicked, [this] {
        this->mapListFont = PorymapConfig::defaultMapListFont();
        QToolTip::showText(ui->pushButton_ResetMapListFont->mapToGlobal(QPoint(0, 0)), "Font reset!");
    });

    initFields();
    updateFields();
}

PreferenceEditor::~PreferenceEditor()
{
    delete ui;
}

void PreferenceEditor::initFields() {
    QStringList themes = { "default" };
    static const QRegularExpression re(":/themes/([A-z0-9_-]+).qss");
    QDirIterator it(":/themes", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString themeName = re.match(it.next()).captured(1);
        if (!themes.contains(themeName)) {
            themes.append(themeName);
        }
    }
    themeSelector->addItems(themes);
}

void PreferenceEditor::updateFields() {
    themeSelector->setTextItem(porymapConfig.theme);
    if (porymapConfig.eventSelectionShapeMode == QGraphicsPixmapItem::MaskShape) {
        ui->radioButton_OnSprite->setChecked(true);
    } else if (porymapConfig.eventSelectionShapeMode == QGraphicsPixmapItem::BoundingRectShape) {
        ui->radioButton_WithinRect->setChecked(true);
    }
    ui->lineEdit_TextEditorOpenFolder->setText(porymapConfig.textEditorOpenFolder);
    ui->lineEdit_TextEditorGotoLine->setText(porymapConfig.textEditorGotoLine);
    ui->checkBox_MonitorProjectFiles->setChecked(porymapConfig.monitorFiles);
    ui->checkBox_OpenRecentProject->setChecked(porymapConfig.reopenOnLaunch);
    ui->checkBox_CheckForUpdates->setChecked(porymapConfig.checkForUpdates);
    ui->checkBox_DisableEventWarning->setChecked(porymapConfig.eventDeleteWarningDisabled);

    if (porymapConfig.scriptAutocompleteMode == ScriptAutocompleteMode::MapOnly) {
        ui->radioButton_AutocompleteMapScripts->setChecked(true);
    } else if (porymapConfig.scriptAutocompleteMode == ScriptAutocompleteMode::MapAndCommon) {
        ui->radioButton_AutocompleteCommonScripts->setChecked(true);
    } else if (porymapConfig.scriptAutocompleteMode == ScriptAutocompleteMode::All) {
        ui->radioButton_AutocompleteAllScripts->setChecked(true);
    }

    auto logTypeEnd = porymapConfig.statusBarLogTypes.end();
    ui->checkBox_StatusErrors->setChecked(porymapConfig.statusBarLogTypes.find(LogType::LOG_ERROR) != logTypeEnd);
    ui->checkBox_StatusWarnings->setChecked(porymapConfig.statusBarLogTypes.find(LogType::LOG_WARN) != logTypeEnd);
    ui->checkBox_StatusInformation->setChecked(porymapConfig.statusBarLogTypes.find(LogType::LOG_INFO) != logTypeEnd);

    this->applicationFont = porymapConfig.applicationFont;
    this->mapListFont = porymapConfig.mapListFont;
}

void PreferenceEditor::saveFields() {
    bool needsProjectReload = false;

    bool changedTheme = false;
    if (themeSelector->currentText() != porymapConfig.theme) {
        porymapConfig.theme = themeSelector->currentText();
        changedTheme = true;
    }

    auto scriptAutocompleteMode = ScriptAutocompleteMode::MapOnly;
    if (ui->radioButton_AutocompleteCommonScripts->isChecked()) {
        scriptAutocompleteMode = ScriptAutocompleteMode::MapAndCommon;
    } else if (ui->radioButton_AutocompleteAllScripts->isChecked()) {
        scriptAutocompleteMode = ScriptAutocompleteMode::All;
    }
    if (scriptAutocompleteMode != porymapConfig.scriptAutocompleteMode) {
        porymapConfig.scriptAutocompleteMode = scriptAutocompleteMode;
        emit scriptSettingsChanged(scriptAutocompleteMode);
    }

    porymapConfig.eventSelectionShapeMode = ui->radioButton_OnSprite->isChecked() ? QGraphicsPixmapItem::MaskShape : QGraphicsPixmapItem::BoundingRectShape;
    porymapConfig.textEditorOpenFolder = ui->lineEdit_TextEditorOpenFolder->text();
    porymapConfig.textEditorGotoLine = ui->lineEdit_TextEditorGotoLine->text();
    porymapConfig.reopenOnLaunch = ui->checkBox_OpenRecentProject->isChecked();
    porymapConfig.checkForUpdates = ui->checkBox_CheckForUpdates->isChecked();
    porymapConfig.eventDeleteWarningDisabled = ui->checkBox_DisableEventWarning->isChecked();

    porymapConfig.statusBarLogTypes.clear();
    if (ui->checkBox_StatusErrors->isChecked()) porymapConfig.statusBarLogTypes.insert(LogType::LOG_ERROR);
    if (ui->checkBox_StatusWarnings->isChecked()) porymapConfig.statusBarLogTypes.insert(LogType::LOG_WARN);
    if (ui->checkBox_StatusInformation->isChecked()) porymapConfig.statusBarLogTypes.insert(LogType::LOG_INFO);

    if (porymapConfig.monitorFiles != ui->checkBox_MonitorProjectFiles->isChecked()) {
        porymapConfig.monitorFiles = ui->checkBox_MonitorProjectFiles->isChecked();
        needsProjectReload = true;
    }

    if (porymapConfig.applicationFont != this->applicationFont) {
        porymapConfig.applicationFont = this->applicationFont;
        changedTheme = true;
    }
    if (porymapConfig.mapListFont != this->mapListFont) {
        porymapConfig.mapListFont = this->mapListFont;
        changedTheme = true;
    }

    if (changedTheme) {
        emit themeChanged(porymapConfig.theme);
    }

    porymapConfig.save();
    emit preferencesSaved();

    if (needsProjectReload) {
        auto message = QStringLiteral("Some changes will only take effect after reloading the project. Reload the project now?");
        if (QuestionMessage::show(message, this) == QMessageBox::Yes) {
            emit reloadProjectRequested();
        }
    }
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

void PreferenceEditor::openFontDialog(QFont *font) {
    auto dialog = new QFontDialog(*font, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setOption(QFontDialog::DontUseNativeDialog);
    connect(dialog, &QFontDialog::fontSelected, [font](const QFont &selectedFont) {
        *font = selectedFont;
    });
    dialog->open();
}
