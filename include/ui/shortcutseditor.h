#ifndef SHORTCUTSEDITOR_H
#define SHORTCUTSEDITOR_H

#include <QDialog>
#include <QMap>

class QAbstractButton;
class QAction;
class QKeySequenceEdit;
class ActionShortcutEdit;


namespace Ui {
class ShortcutsEditor;
}

class ShortcutsEditor : public QDialog
{
    Q_OBJECT

public:
    explicit ShortcutsEditor(QWidget *parent = nullptr);
    ~ShortcutsEditor();

private:
    Ui::ShortcutsEditor *ui;
    QMap<QString, QAction *> actions;
    QWidget *ase_container;

    void populateShortcuts();
    void saveShortcuts();
    void resetShortcuts();
    void promptUser(ActionShortcutEdit *current, ActionShortcutEdit *sender);

private slots:
    void checkForDuplicates();
    void dialogButtonClicked(QAbstractButton *button);
};


// A collection of QKeySequenceEdit's in a QHBoxLayout with a cooresponding QAction
class ActionShortcutEdit : public QWidget
{
    Q_OBJECT

public:
    explicit ActionShortcutEdit(QWidget *parent = nullptr, QAction *action = nullptr, int count = 1);

    bool eventFilter(QObject *watched, QEvent *event) override;

    int count() const { return kse_children.count(); }
    void setCount(int count);
    QList<QKeySequence> shortcuts() const;
    void setShortcuts(const QList<QKeySequence> &keySequences);
    void applyShortcuts();
    bool removeOne(const QKeySequence &keySequence);
    bool contains(const QKeySequence &keySequence);
    bool contains(QKeySequenceEdit *keySequenceEdit);
    QKeySequence last() const { return shortcuts().last(); }

    QAction *action;

public slots:
    void clear();

signals:
    void editingFinished();

private:
    QVector<QKeySequenceEdit *> kse_children;
    QList<QKeySequence> ks_list;

    void updateShortcuts() { setShortcuts(shortcuts()); }
    void focusLast();

private slots:
    void onEditingFinished();
};

#endif // SHORTCUTSEDITOR_H
