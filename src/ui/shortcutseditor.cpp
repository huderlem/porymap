#include "shortcutseditor.h"
#include "ui_shortcutseditor.h"
#include "config.h"
#include "multikeyedit.h"
#include "message.h"
#include "log.h"

#include <QGroupBox>
#include <QFormLayout>
#include <QAbstractButton>
#include <QtEvents>
#include <QMessageBox>
#include <QRegularExpression>
#include <QLabel>
#include <QMenu>


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
        if (!it.value()) {
            // Some shortcuts cannot be saved. Pointers in the object map can become null if they are
            // deleted externally while the shortcuts editor is open. Ideally we should try to restore
            // the original object so the shortcut can be saved. Alternatively this could generally be
            // prevented by making the shortcuts editor modal. For now, saving these shortcuts is skipped,
            // and we warn the user that this happened.
            ErrorMessage::show(QStringLiteral("Some shortcuts failed to save. Please close the Shortcuts Editor and retry."), this);
            return;
        }
        if (it.key()->keySequences().isEmpty())
            objects_keySequences.insert(it.value(), QKeySequence());
        for (auto keySequence : it.key()->keySequences())
            objects_keySequences.insert(it.value(), keySequence);
    }

    shortcutsConfig.setUserShortcuts(objects_keySequences);
    shortcutsConfig.save();
    emit shortcutsSaved();
}

// Restores default shortcuts but doesn't save until Apply or OK is clicked.
void ShortcutsEditor::resetShortcuts() {
    for (auto it = multiKeyEdits_objects.begin(); it != multiKeyEdits_objects.end(); ++it) {
        if (!it.value()) continue;
        it.key()->blockSignals(true);
        const auto defaults = shortcutsConfig.defaultShortcuts(it.value());
        it.key()->setKeySequences(defaults);
        it.key()->blockSignals(false);
    }
}

void ShortcutsEditor::parseObject(const QObject *object, QMap<const QObject*, QString> *objects_labels, QMap<const QObject*, QString> *objects_prefixes) {
    auto menu = dynamic_cast<const QMenu*>(object);
    if (menu) {
        // If a menu is provided we'll use it to create prefixes for any of the menu's actions,
        // and automatically insert its actions in the shortcut list (if they weren't present already).
        // The prefixing assumes the provided object list is in inheritance order.
        // These prefixes are important for differentiating actions that may have the same display text
        // but appear in different menus.
        for (const auto &action : menu->actions()) {
            if (!menu->title().isEmpty()) {
                auto prefix = QString("%1%2 > ")
                                        .arg(objects_prefixes->value(menu->menuAction())) // If this is a sub-menu, it may itself have a prefix.
                                        .arg(menu->title());
                objects_prefixes->insert(action, prefix);
            }
            parseObject(action, objects_labels, objects_prefixes);
        }
    } else if (object && !object->objectName().isEmpty() && !object->objectName().startsWith("_q_")) {
        QString label = getLabel(object);
        if (!label.isEmpty()) {
            objects_labels->insert(object, label);
        }
    }
}

void ShortcutsEditor::parseObjectList(const QObjectList &objectList) {
    QMap<const QObject*, QString> objects_labels;
    QMap<const QObject*, QString> objects_prefixes;
    for (const auto &object : objectList) {
        parseObject(object, &objects_labels, &objects_prefixes);
    }

    // Sort alphabetically by label
    this->labels_objects.clear();
    for (auto it = objects_labels.constBegin(); it != objects_labels.constEnd(); it++) {
        QString fullLabel = objects_prefixes.value(it.key()) + it.value();
        this->labels_objects.insert(fullLabel, it.key());
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
        if (!object) continue;
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
    static const QRegularExpression re("[A-Z]");
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

    for (auto *sibling_multiKeyEdit : siblings(sender_multiKeyEdit)) {
        if (sibling_multiKeyEdit->contains(keySequence)) {
            promptUserOnDuplicateFound(sender_multiKeyEdit, sibling_multiKeyEdit);
            break;
        }
    }
}

QList<MultiKeyEdit *> ShortcutsEditor::siblings(MultiKeyEdit *multiKeyEdit) const {
    auto list = multiKeyEdit->parent()->findChildren<MultiKeyEdit *>(QString(), Qt::FindDirectChildrenOnly);
    list.removeOne(multiKeyEdit);
    return list;
}

void ShortcutsEditor::promptUserOnDuplicateFound(MultiKeyEdit *sender, MultiKeyEdit *sibling) {
    const auto duplicateKeySequence = sender->keySequences().last();
    const auto siblingLabel = this->labels_objects.key(multiKeyEdits_objects.value(sibling));
    if (siblingLabel.isEmpty())
        return;
    const auto message = QString("Shortcut '%1' is already used by '%2', would you like to replace it?")
                                    .arg(duplicateKeySequence.toString())
                                    .arg(siblingLabel);

    // QKeySequenceEdits::keySequenceChange fires when editing finishes on a QKeySequenceEdit,
    // even if no change occurs. Displaying our question prompt will cause the edit to lose focus
    // and fire another signal, which would cause another "duplicate shortcut" prompt to appear.
    // For this reason we need to block their signals before the message is displayed.
    const QSignalBlocker b_Sender(sender);
    const QSignalBlocker b_Sibling(sibling);

    if (QuestionMessage::show(message, this) == QMessageBox::Yes) {
        sibling->removeOne(duplicateKeySequence);
    } else {
        sender->removeOne(duplicateKeySequence);
    }
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
