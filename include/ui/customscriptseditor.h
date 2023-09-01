#ifndef CUSTOMSCRIPTSEDITOR_H
#define CUSTOMSCRIPTSEDITOR_H

#include <QMainWindow>
#include <QListWidget>
#include <QAbstractButton>
#include <QMessageBox>

#include "customscriptslistitem.h"

namespace Ui {
class CustomScriptsEditor;
}


class CustomScriptsEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit CustomScriptsEditor(QWidget *parent = nullptr);
    ~CustomScriptsEditor();

signals:
    void reloadScriptEngine();

private:
    Ui::CustomScriptsEditor *ui;

    bool hasUnsavedChanges = false;
    QString importDir;
    const QString baseDir;

    void displayScript(const QString &filepath);
    QString chooseScript(QString dir);

    void removeScript(QListWidgetItem * item);
    void replaceScript(QListWidgetItem * item);
    void openScript(QListWidgetItem * item);

    QString getListItemFilepath(QListWidgetItem * item) const;
    void setListItemFilepath(QListWidgetItem * item, QString filepath) const;

    int prompt(const QString &text, QMessageBox::StandardButton defaultButton);
    void save();
    void closeEvent(QCloseEvent*);

    void initShortcuts();
    QObjectList shortcutableObjects() const;
    void applyUserShortcuts();

private slots:
    void dialogButtonClicked(QAbstractButton *button);
    void addNewScript();
    void reloadScripts();
    void removeSelectedScripts();
    void openSelectedScripts();
};

#endif // CUSTOMSCRIPTSEDITOR_H
