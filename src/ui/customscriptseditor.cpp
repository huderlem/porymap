#include "customscriptseditor.h"
#include "ui_customscriptseditor.h"
#include "ui_customscriptslistitem.h"
#include "config.h"
#include "editor.h"
#include "shortcut.h"

#include <QDir>
#include <QFileDialog>

CustomScriptsEditor::CustomScriptsEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CustomScriptsEditor),
    baseDir(userConfig.getProjectDir() + QDir::separator())
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    // This property seems to be reset if we don't set it programmatically
    ui->list->setDragDropMode(QAbstractItemView::NoDragDrop);

    const QStringList paths = userConfig.getCustomScriptPaths();
    const QList<bool> enabled = userConfig.getCustomScriptsEnabled();
    for (int i = 0; i < paths.length(); i++)
        this->displayScript(paths.at(i), enabled.at(i));

    this->fileDialogDir = userConfig.getProjectDir();

    connect(ui->button_CreateNewScript, &QAbstractButton::clicked, this, &CustomScriptsEditor::createNewScript);
    connect(ui->button_LoadScript, &QAbstractButton::clicked, this, &CustomScriptsEditor::loadScript);
    connect(ui->button_RefreshScripts, &QAbstractButton::clicked, this, &CustomScriptsEditor::refreshScripts);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &CustomScriptsEditor::dialogButtonClicked);

    this->initShortcuts();
    this->restoreWindowState();
}

CustomScriptsEditor::~CustomScriptsEditor()
{
    ui->list->clear();
    delete ui;
}

void CustomScriptsEditor::initShortcuts() {
    auto *shortcut_remove = new Shortcut({QKeySequence("Del"), QKeySequence("Backspace")}, this, SLOT(removeSelectedScripts()));
    shortcut_remove->setObjectName("shortcut_remove");
    shortcut_remove->setWhatsThis("Remove Selected Scripts");

    auto *shortcut_open = new Shortcut(QKeySequence(), this, SLOT(openSelectedScripts()));
    shortcut_open->setObjectName("shortcut_open");
    shortcut_open->setWhatsThis("Open Selected Scripts");

    auto *shortcut_createNew = new Shortcut(QKeySequence(), this, SLOT(createNewScript()));
    shortcut_createNew->setObjectName("shortcut_createNew");
    shortcut_createNew->setWhatsThis("Create New Script...");

    auto *shortcut_load = new Shortcut(QKeySequence(), this, SLOT(loadScript()));
    shortcut_load->setObjectName("shortcut_load");
    shortcut_load->setWhatsThis("Load Script...");

    auto *shortcut_refresh = new Shortcut(QKeySequence(), this, SLOT(refreshScripts()));
    shortcut_refresh->setObjectName("shortcut_refresh");
    shortcut_refresh->setWhatsThis("Refresh Scripts");

    shortcutsConfig.load();
    shortcutsConfig.setDefaultShortcuts(shortcutableObjects());
    applyUserShortcuts();
}

QObjectList CustomScriptsEditor::shortcutableObjects() const {
    QObjectList shortcutable_objects;

    for (auto *action : findChildren<QAction *>())
        if (!action->objectName().isEmpty())
            shortcutable_objects.append(qobject_cast<QObject *>(action));
    for (auto *shortcut : findChildren<Shortcut *>())
        if (!shortcut->objectName().isEmpty())
            shortcutable_objects.append(qobject_cast<QObject *>(shortcut));

    return shortcutable_objects;
}

void CustomScriptsEditor::applyUserShortcuts() {
    for (auto *action : findChildren<QAction *>())
        if (!action->objectName().isEmpty())
            action->setShortcuts(shortcutsConfig.userShortcuts(action));
    for (auto *shortcut : findChildren<Shortcut *>())
        if (!shortcut->objectName().isEmpty())
            shortcut->setKeys(shortcutsConfig.userShortcuts(shortcut));
}

void CustomScriptsEditor::restoreWindowState() {
    logInfo("Restoring custom scripts editor geometry from previous session.");
    const QMap<QString, QByteArray> geometry = porymapConfig.getCustomScriptsEditorGeometry();
    this->restoreGeometry(geometry.value("custom_scripts_editor_geometry"));
    this->restoreState(geometry.value("custom_scripts_editor_state"));
}

void CustomScriptsEditor::displayScript(const QString &filepath, bool enabled) {
    auto item = new QListWidgetItem();
    auto widget = new CustomScriptsListItem();

    widget->ui->checkBox_Enable->setChecked(enabled);
    widget->ui->lineEdit_filepath->setText(filepath);
    item->setSizeHint(widget->sizeHint());

    connect(widget->ui->b_Choose, &QAbstractButton::clicked, [this, item](bool) { this->replaceScript(item); });
    connect(widget->ui->b_Edit,   &QAbstractButton::clicked, [this, item](bool) { this->openScript(item); });
    connect(widget->ui->b_Delete, &QAbstractButton::clicked, [this, item](bool) { this->removeScript(item); });
    connect(widget->ui->checkBox_Enable, &QCheckBox::stateChanged, this, &CustomScriptsEditor::markEdited);
    connect(widget->ui->lineEdit_filepath, &QLineEdit::textEdited, this, &CustomScriptsEditor::markEdited);

    // Per the Qt manual, for performance reasons QListWidget::setItemWidget shouldn't be used with non-static items.
    // There's an assumption here that users won't have enough scripts for that to be a problem.
    ui->list->addItem(item);
    ui->list->setItemWidget(item, widget);
}

void CustomScriptsEditor::markEdited() {
    this->hasUnsavedChanges = true;
}

QString CustomScriptsEditor::getScriptFilepath(QListWidgetItem * item, bool absolutePath) const {
    auto widget = dynamic_cast<CustomScriptsListItem *>(ui->list->itemWidget(item));
    if (!widget) return QString();

    QString path = widget->ui->lineEdit_filepath->text();
    if (absolutePath) {
        QFileInfo fileInfo(path);
        if (fileInfo.isRelative())
            path.prepend(this->baseDir);
    }
    return path;
}

void CustomScriptsEditor::setScriptFilepath(QListWidgetItem * item, QString filepath) const {
    auto widget = dynamic_cast<CustomScriptsListItem *>(ui->list->itemWidget(item));
    if (!widget) return;

    if (filepath.startsWith(this->baseDir))
        filepath.remove(0, this->baseDir.length());
    widget->ui->lineEdit_filepath->setText(filepath);
}

bool CustomScriptsEditor::getScriptEnabled(QListWidgetItem * item) const {
    auto widget = dynamic_cast<CustomScriptsListItem *>(ui->list->itemWidget(item));
    return widget && widget->ui->checkBox_Enable->isChecked();
}

QString CustomScriptsEditor::chooseScript(QString dir) {
    return QFileDialog::getOpenFileName(this, "Choose Custom Script File", dir, "JavaScript Files (*.js)");
}

void CustomScriptsEditor::createNewScript() {
    QString filepath = QFileDialog::getSaveFileName(this, "Create New Script File", this->fileDialogDir + "/new_script.js", "JavaScript Files (*.js)");

    // QFileDialog::getSaveFileName returns focus to the main editor window when closed. Workaround for this below
    this->raise();
    this->activateWindow();

    if (filepath.isEmpty())
        return;
    this->fileDialogDir = filepath;

    QFile scriptFile(filepath);
    if (!scriptFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(filepath));
        QMessageBox messageBox(this);
        messageBox.setText("Failed to create new script file!");
        messageBox.setInformativeText(QString("Could not open \"%1\" for writing").arg(filepath));
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.exec();
        return;
    }
    ParseUtil parser;
    scriptFile.write(parser.readTextFile(":/text/script_template.txt").toUtf8());
    scriptFile.close();

    this->displayNewScript(filepath);
}

void CustomScriptsEditor::loadScript() {
    QString filepath = this->chooseScript(this->fileDialogDir);
    if (filepath.isEmpty())
        return;
    this->fileDialogDir = filepath;
    this->displayNewScript(filepath);
}

void CustomScriptsEditor::displayNewScript(QString filepath) {
    if (filepath.startsWith(this->baseDir))
        filepath.remove(0, this->baseDir.length());

    // Verify new script path is not already in list
    for (int i = 0; i < ui->list->count(); i++) {
        if (filepath == this->getScriptFilepath(ui->list->item(i), false)) {
            QMessageBox::information(this, "", QString("The script '%1' is already loaded").arg(filepath));
            return;
        }
    }

    this->displayScript(filepath, true);
    this->markEdited();
}

void CustomScriptsEditor::removeScript(QListWidgetItem * item) {
    ui->list->takeItem(ui->list->row(item));
    this->markEdited();
}

void CustomScriptsEditor::removeSelectedScripts() {
    QList<QListWidgetItem *> items = ui->list->selectedItems();
    if (items.length() == 0)
        return;
    for (auto item : items)
        this->removeScript(item);
}

void CustomScriptsEditor::replaceScript(QListWidgetItem * item) {
    const QString filepath = this->chooseScript(this->getScriptFilepath(item));
    if (filepath.isEmpty())
        return;
    this->setScriptFilepath(item, filepath);
    this->markEdited();
}

void CustomScriptsEditor::openScript(QListWidgetItem * item) {
    const QString path = this->getScriptFilepath(item);
    QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()){
        QMessageBox::warning(this, "", QString("Failed to open script '%1'").arg(path));
        return;
    }
    Editor::openInTextEditor(path);
}

void CustomScriptsEditor::openSelectedScripts() {
    for (auto item : ui->list->selectedItems())
        this->openScript(item);
}

void CustomScriptsEditor::refreshScripts() {
    if (this->hasUnsavedChanges) {
        if (this->prompt("Scripts have been modified, save changes and reload the script engine?", QMessageBox::Yes) == QMessageBox::No)
            return;
        this->save();
    }
    QToolTip::showText(ui->button_RefreshScripts->mapToGlobal(QPoint(0, 0)), "Refreshed!");
    emit reloadScriptEngine();
}

void CustomScriptsEditor::save() {
    if (!this->hasUnsavedChanges)
        return;

    QStringList paths;
    QList<bool> enabledStates;
    for (int i = 0; i < ui->list->count(); i++) {
        auto item = ui->list->item(i);
        const QString path = this->getScriptFilepath(item, false);
        if (!path.isEmpty()) {
            paths.append(path);
            enabledStates.append(this->getScriptEnabled(item));
        }
    }

    userConfig.setCustomScripts(paths, enabledStates);
    this->hasUnsavedChanges = false;
    this->refreshScripts();
}

int CustomScriptsEditor::prompt(const QString &text, QMessageBox::StandardButton defaultButton) {
    QMessageBox messageBox(this);
    messageBox.setText(text);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | defaultButton);
    messageBox.setDefaultButton(defaultButton);
    return messageBox.exec();
}

void CustomScriptsEditor::dialogButtonClicked(QAbstractButton *button) {
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
        this->save();
    close(); // All buttons (OK and Cancel) close the window
}

void CustomScriptsEditor::closeEvent(QCloseEvent* event) {
    if (this->hasUnsavedChanges) {
        int result = this->prompt("Scripts have been modified, save changes?", QMessageBox::Cancel);
        if (result == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
        if (result == QMessageBox::Yes)
            this->save();
    }

    porymapConfig.setCustomScriptsEditorGeometry(
        this->saveGeometry(),
        this->saveState()
    );
}
