#include "shortcutseditor.h"
#include "ui_shortcutseditor.h"
#include "config.h"
#include "multikeyedit.h"
#include "log.h"

#include <QGroupBox>
#include <QFormLayout>
#include <QAbstractButton>
#include <QtEvents>
#include <QMessageBox>
#include <QRegularExpression>
#include <QLabel>


ShortcutsEditor::ShortcutsEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ShortcutsEditor),
    main_container(nullptr)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    main_container = ui->scrollAreaWidgetContents_Shortcuts;
    auto *main_layout = new QVBoxLayout(main_container);
    main_layout->setSpacing(12);
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &ShortcutsEditor::dialogButtonClicked);
}

ShortcutsEditor::ShortcutsEditor(const QObjectList &shortcutableObjects, QWidget *parent) :
    ShortcutsEditor(parent)
{
    setShortcutableObjects(shortcutableObjects);
}

ShortcutsEditor::~ShortcutsEditor()
{
    delete ui;
}

void ShortcutsEditor::setShortcutableObjects(const QObjectList &shortcutableObjects) {
    parseObjectList(shortcutableObjects);
    populateMainContainer();
}

void ShortcutsEditor::saveShortcuts() {
    QMultiMap<const QObject *, QKeySequence> objects_keySequences;
    for (auto it = multiKeyEdits_objects.cbegin(); it != multiKeyEdits_objects.cend(); ++it) {
        if (it.key()->keySequences().isEmpty())
            objects_keySequences.insert(it.value(), QKeySequence());
        for (auto keySequence : it.key()->keySequences())
            objects_keySequences.insert(it.value(), keySequence);
    }

    shortcutsConfig.setUserShortcuts(objects_keySequences);
    emit shortcutsSaved();
}

// Restores default shortcuts but doesn't save until Apply or OK is clicked.
void ShortcutsEditor::resetShortcuts() {
    for (auto it = multiKeyEdits_objects.begin(); it != multiKeyEdits_objects.end(); ++it) {
        it.key()->blockSignals(true);
        const auto defaults = shortcutsConfig.defaultShortcuts(it.value());
        it.key()->setKeySequences(defaults);
        it.key()->blockSignals(false);
    }
}

void ShortcutsEditor::parseObjectList(const QObjectList &objectList) {
    for (auto *object : objectList) {
        const auto label = getLabel(object);
        if (!label.isEmpty() && !object->objectName().isEmpty() && !object->objectName().startsWith("_q_"))
            labels_objects.insert(label, object);
    }
}

QString ShortcutsEditor::getLabel(const QObject *object) const {
    if (stringPropertyIsNotEmpty(object, "text"))
        return object->property("text").toString().remove('&');
    else if (stringPropertyIsNotEmpty(object, "whatsThis"))
        return object->property("whatsThis").toString().remove('&');
    else
        return QString();
}

bool ShortcutsEditor::stringPropertyIsNotEmpty(const QObject *object, const char *name) const {
    return object->property(name).isValid() && !object->property(name).toString().isEmpty();
}

void ShortcutsEditor::populateMainContainer() {
    for (auto object : labels_objects) {
        const auto shortcutContext = getShortcutContext(object);
        if (!contexts_layouts.contains(shortcutContext))
            addNewContextGroup(shortcutContext);

        addNewMultiKeyEdit(object, shortcutContext);
    }
}

// The context for which the object's shortcut is active (Displayed in group box title).
// Uses the parent window's objectName and adds spaces between words.
QString ShortcutsEditor::getShortcutContext(const QObject *object) const {
    auto objectParentWidget = static_cast<QWidget *>(object->parent());
    auto context = objectParentWidget->window()->objectName();
    QRegularExpression re("[A-Z]");
    int i = context.indexOf(re, 1);
    while (i != -1) {
        if (context.at(i - 1) != ' ')
            context.insert(i++, ' ');
        i = context.indexOf(re, i + 1);
    }
    return context;
}

// Seperate shortcuts into context groups for duplicate checking.
void ShortcutsEditor::addNewContextGroup(const QString &shortcutContext) {
    auto *groupBox = new QGroupBox(shortcutContext, main_container);
    main_container->layout()->addWidget(groupBox);
    auto *formLayout = new QFormLayout(groupBox);
    contexts_layouts.insert(shortcutContext, formLayout);
}

void ShortcutsEditor::addNewMultiKeyEdit(const QObject *object, const QString &shortcutContext) {
    auto *container = contexts_layouts.value(shortcutContext)->parentWidget();
    auto *multiKeyEdit = new MultiKeyEdit(container);
    multiKeyEdit->setKeySequences(shortcutsConfig.userShortcuts(object));
    connect(multiKeyEdit, &MultiKeyEdit::keySequenceChanged,
            this, &ShortcutsEditor::checkForDuplicates);
    contexts_layouts.value(shortcutContext)->addRow(labels_objects.key(object), multiKeyEdit);
    multiKeyEdits_objects.insert(multiKeyEdit, object);
}

void ShortcutsEditor::checkForDuplicates(const QKeySequence &keySequence) {
    if (keySequence.isEmpty())
        return;

    auto *sender_multiKeyEdit = qobject_cast<MultiKeyEdit *>(sender());
    if (!sender_multiKeyEdit)
        return;

    for (auto *sibling_multiKeyEdit : siblings(sender_multiKeyEdit))
        if (sibling_multiKeyEdit->contains(keySequence))
            promptUserOnDuplicateFound(sender_multiKeyEdit, sibling_multiKeyEdit);
}

QList<MultiKeyEdit *> ShortcutsEditor::siblings(MultiKeyEdit *multiKeyEdit) const {
    auto list = multiKeyEdit->parent()->findChildren<MultiKeyEdit *>(QString(), Qt::FindDirectChildrenOnly);
    list.removeOne(multiKeyEdit);
    return list;
}

void ShortcutsEditor::promptUserOnDuplicateFound(MultiKeyEdit *sender, MultiKeyEdit *sibling) {
    const auto duplicateKeySequence = sender->keySequences().last();
    const auto siblingLabel = getLabel(multiKeyEdits_objects.value(sibling));
    const auto message = QString(
            "Shortcut '%1' is already used by '%2', would you like to replace it?")
            .arg(duplicateKeySequence.toString()).arg(siblingLabel);

    const auto result = QMessageBox::question(
            this, "porymap", message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result == QMessageBox::Yes)
        removeKeySequence(duplicateKeySequence, sibling);
    else
        removeKeySequence(duplicateKeySequence, sender);
    
    activateWindow();
}

void ShortcutsEditor::removeKeySequence(const QKeySequence &keySequence, MultiKeyEdit *multiKeyEdit) {
    multiKeyEdit->blockSignals(true);
    multiKeyEdit->removeOne(keySequence);
    multiKeyEdit->blockSignals(false);
}

void ShortcutsEditor::dialogButtonClicked(QAbstractButton *button) {
    auto buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::AcceptRole) {
        saveShortcuts();
        close();
    } else if (buttonRole == QDialogButtonBox::ApplyRole) {
        saveShortcuts();
    } else if (buttonRole == QDialogButtonBox::RejectRole) {
        close();
    } else if (buttonRole == QDialogButtonBox::ResetRole) {
        resetShortcuts();
    }
}
