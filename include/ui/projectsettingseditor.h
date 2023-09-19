#ifndef PROJECTSETTINGSEDITOR_H
#define PROJECTSETTINGSEDITOR_H

#include <QMainWindow>
#include "project.h"

class NoScrollComboBox;
class QAbstractButton;


namespace Ui {
class ProjectSettingsEditor;
}

class ProjectSettingsEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit ProjectSettingsEditor(QWidget *parent = nullptr, Project *project = nullptr);
    ~ProjectSettingsEditor();

signals:
    void reloadProject();

private:
    Ui::ProjectSettingsEditor *ui;
    Project *project;

    bool hasUnsavedChanges = false;
    bool projectNeedsReload = false;
    bool refreshing = false;
    const QString baseDir;

    void initUi();
    void connectSignals();
    void restoreWindowState();
    void save();
    void refresh();
    void closeEvent(QCloseEvent*);
    int prompt(const QString &, QMessageBox::StandardButton = QMessageBox::Yes);
    bool promptSaveChanges();
    bool promptRestoreDefaults();

    void setBorderMetatilesUi(bool customSize);
    void setBorderMetatileIds(bool customSize, QList<uint16_t> metatileIds);
    QList<uint16_t> getBorderMetatileIds(bool customSize);

    void createProjectPathsTable();
    QString chooseProjectFile(const QString &defaultFilepath);

private slots:
    void dialogButtonClicked(QAbstractButton *button);
    void choosePrefabsFileClicked(bool);
    void importDefaultPrefabsClicked(bool);
    void updateAttributeLimits(const QString &attrSize);
    void markEdited();
};

#endif // PROJECTSETTINGSEDITOR_H
