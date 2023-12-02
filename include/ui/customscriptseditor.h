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

public slots:
    void applyUserShortcuts();

private:
    Ui::CustomScriptsEditor *ui;

    bool hasUnsavedChanges = false;
    QString fileDialogDir;
    const QString baseDir;

    void displayScript(const QString &filepath, bool enabled);
    void displayNewScript(QString filepath);
    QString chooseScript(QString dir);
    void removeScript(QListWidgetItem * item);
    void replaceScript(QListWidgetItem * item);
    void openScript(QListWidgetItem * item);
    QString getScriptFilepath(QListWidgetItem * item, bool absolutePath = true) const;
    void setScriptFilepath(QListWidgetItem * item, QString filepath) const;
    bool getScriptEnabled(QListWidgetItem * item) const;
    void markEdited();
    int prompt(const QString &text, QMessageBox::StandardButton defaultButton);
    void save();
    void closeEvent(QCloseEvent*);
    void restoreWindowState();
    void initShortcuts();
    QObjectList shortcutableObjects() const;

private slots:
    void dialogButtonClicked(QAbstractButton *button);
    void createNewScript();
    void loadScript();
    void refreshScripts();
    void removeSelectedScripts();
    void openSelectedScripts();
};

#endif // CUSTOMSCRIPTSEDITOR_H
