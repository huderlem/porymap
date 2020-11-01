#include "shortcutseditor.h"
#include "ui_shortcutseditor.h"
#include "config.h"
#include "log.h"

#include <QFormLayout>
#include <QAbstractButton>
#include <QtEvents>
#include <QMessageBox>
#include <QKeySequenceEdit>
#include <QAction>


ShortcutsEditor::ShortcutsEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ShortcutsEditor),
    actions(QMap<QString, QAction *>())
{
    ui->setupUi(this);
    ase_container = ui->scrollAreaWidgetContents_Shortcuts;
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &ShortcutsEditor::dialogButtonClicked);
    populateShortcuts();
}

ShortcutsEditor::~ShortcutsEditor()
{
    delete ui;
}

void ShortcutsEditor::populateShortcuts() {
    if (!parent())
        return;

    for (auto action : parent()->findChildren<QAction *>())
        if (!action->text().isEmpty() && !action->objectName().isEmpty())
            actions.insert(action->text().remove('&'), action);

    auto *formLayout = new QFormLayout(ase_container);
    for (auto *action : actions) {
        auto userShortcuts = shortcutsConfig.getUserShortcuts(action);
        auto *ase = new ActionShortcutEdit(ase_container, action, 2);
        connect(ase, &ActionShortcutEdit::editingFinished,
                this, &ShortcutsEditor::checkForDuplicates);
        ase->setShortcuts(userShortcuts);
        formLayout->addRow(action->text(), ase);
    }
}

void ShortcutsEditor::saveShortcuts() {
    auto ase_children = ase_container->findChildren<ActionShortcutEdit *>(QString(), Qt::FindDirectChildrenOnly);
    for (auto *ase : ase_children)
        ase->applyShortcuts();
    shortcutsConfig.setUserShortcuts(actions.values());
}

void ShortcutsEditor::resetShortcuts() {
    auto ase_children = ase_container->findChildren<ActionShortcutEdit *>(QString(), Qt::FindDirectChildrenOnly);
    for (auto *ase : ase_children)
        ase->setShortcuts(shortcutsConfig.getDefaultShortcuts(ase->action));
}

void ShortcutsEditor::promptUser(ActionShortcutEdit *current, ActionShortcutEdit *sender) {
    auto result = QMessageBox::question(
        this,
        "porymap",
        QString("Shortcut \"%1\" is already used by \"%2\", would you like to replace it?")
            .arg(sender->last().toString()).arg(current->action->text()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (result == QMessageBox::Yes)
        current->removeOne(sender->last());
    else if (result == QMessageBox::No)
        sender->removeOne(sender->last());
}

void ShortcutsEditor::checkForDuplicates() {
    auto *sender_ase = qobject_cast<ActionShortcutEdit *>(sender());
    if (!sender_ase)
        return;

    for (auto *child_kse : findChildren<QKeySequenceEdit *>()) {
        if (child_kse->keySequence().isEmpty() || child_kse->parent() == sender())
            continue;

        if (sender_ase->contains(child_kse->keySequence())) {
            auto *current_ase = qobject_cast<ActionShortcutEdit *>(child_kse->parent());
            if (!current_ase)
                continue;

            promptUser(current_ase, sender_ase);
            activateWindow();
            return;
        }
    }
}

void ShortcutsEditor::dialogButtonClicked(QAbstractButton *button) {
    auto buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::AcceptRole) {
        saveShortcuts();
        hide();
    } else if (buttonRole == QDialogButtonBox::ApplyRole) {
        saveShortcuts();
    } else if (buttonRole == QDialogButtonBox::RejectRole) {
        hide();
    } else if (buttonRole == QDialogButtonBox::ResetRole) {
        resetShortcuts();
    }
}


ActionShortcutEdit::ActionShortcutEdit(QWidget *parent, QAction *action, int count) :
    QWidget(parent),
    action(action),
    kse_children(QVector<QKeySequenceEdit *>()),
    ks_list(QList<QKeySequence>())
{
    setLayout(new QHBoxLayout(this));
    layout()->setContentsMargins(0, 0, 0, 0);
    setCount(count);
}

bool ActionShortcutEdit::eventFilter(QObject *watched, QEvent *event) {
    auto *watched_kse = qobject_cast<QKeySequenceEdit *>(watched);
    if (!watched_kse)
        return false;

    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            watched_kse->clearFocus();
            return true;
        } else if (keyEvent->key() == Qt::Key_Backspace) {
            removeOne(watched_kse->keySequence());
            return true;
        } else {
            watched_kse->clear();
        }
    }
    return false;
}

void ActionShortcutEdit::setCount(int count) {
    if (count < 1)
        count = 1;

    while (kse_children.count() > count) {
        layout()->removeWidget(kse_children.last());
        delete kse_children.last();
        kse_children.removeLast();
    }

    while (kse_children.count() < count) {
        auto *kse = new QKeySequenceEdit(this);
        connect(kse, &QKeySequenceEdit::editingFinished,
                this, &ActionShortcutEdit::onEditingFinished);
        kse->installEventFilter(this);
        layout()->addWidget(kse);
        kse_children.append(kse);
    }
}

QList<QKeySequence> ActionShortcutEdit::shortcuts() const {
    QList<QKeySequence> current_ks_list;
    for (auto *kse : kse_children)
        if (!kse->keySequence().isEmpty())
            current_ks_list.append(kse->keySequence());
    return current_ks_list;
}

void ActionShortcutEdit::setShortcuts(const QList<QKeySequence> &keySequences) {
    clear();
    ks_list = keySequences;
    int minCount = qMin(kse_children.count(), ks_list.count());
    for (int i = 0; i < minCount; ++i)
        kse_children[i]->setKeySequence(ks_list[i]);
}

void ActionShortcutEdit::applyShortcuts() {
    action->setShortcuts(shortcuts());
}

bool ActionShortcutEdit::removeOne(const QKeySequence &keySequence) {
    for (auto *kse : kse_children) {
        if (kse->keySequence() == keySequence) {
            ks_list.removeOne(keySequence);
            kse->clear();
            updateShortcuts();
            return true;
        }
    }
    return false;
}

bool ActionShortcutEdit::contains(const QKeySequence &keySequence) {
    for (auto ks : shortcuts())
        if (ks == keySequence)
            return true;
    return false;
}

bool ActionShortcutEdit::contains(QKeySequenceEdit *keySequenceEdit) {
    for (auto *kse : kse_children)
        if (kse == keySequenceEdit)
            return true;
    return false;
}

void ActionShortcutEdit::clear() {
    for (auto *kse : kse_children)
        kse->clear();
}

void ActionShortcutEdit::focusLast() {
    for (int i = count() - 1; i >= 0; --i) {
        if (!kse_children[i]->keySequence().isEmpty()) {
            kse_children[i]->setFocus();
            return;
        }
    }
}

void ActionShortcutEdit::onEditingFinished() {
    auto *kse = qobject_cast<QKeySequenceEdit *>(sender());
    if (!kse)
        return;

    if (ks_list.contains(kse->keySequence()))
        removeOne(kse->keySequence());
    updateShortcuts();
    focusLast();
    emit editingFinished();
}
