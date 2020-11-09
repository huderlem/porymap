#ifndef SHORTCUTSEDITOR_H
#define SHORTCUTSEDITOR_H

#include "shortcut.h"

#include <QMainWindow>
#include <QDialog>
#include <QMap>
#include <QHash>
#include <QAction>

class QFormLayout;
class MultiKeyEdit;
class QAbstractButton;


namespace Ui {
class ShortcutsEditor;
}

class ShortcutsEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit ShortcutsEditor(QWidget *parent = nullptr);
    explicit ShortcutsEditor(const QObjectList &shortcutableObjects, QWidget *parent = nullptr);
    ~ShortcutsEditor();

    void setShortcutableObjects(const QObjectList &shortcutableObjects);

signals:
    void shortcutsSaved();

private:
    Ui::ShortcutsEditor *ui;
    QWidget *main_container;
    QMultiMap<QString, const QObject *> labels_objects;
    QHash<QString, QFormLayout *> contexts_layouts;
    QHash<MultiKeyEdit *, const QObject *> multiKeyEdits_objects;

    void parseObjectList(const QObjectList &objectList);
    QString getLabel(const QObject *object) const;
    bool stringPropertyIsNotEmpty(const QObject *object, const char *name) const;
    void populateMainContainer();
    QString getShortcutContext(const QObject *object) const;
    void addNewContextGroup(const QString &shortcutContext);
    void addNewMultiKeyEdit(const QObject *object, const QString &shortcutContext);
    QList<MultiKeyEdit *> siblings(MultiKeyEdit *multiKeyEdit) const;
    void promptUserOnDuplicateFound(MultiKeyEdit *current, MultiKeyEdit *sender);
    void removeKeySequence(const QKeySequence &keySequence, MultiKeyEdit *multiKeyEdit);
    void saveShortcuts();
    void resetShortcuts();

private slots:
    void checkForDuplicates(const QKeySequence &keySequence);
    void dialogButtonClicked(QAbstractButton *button);
};

#endif // SHORTCUTSEDITOR_H
