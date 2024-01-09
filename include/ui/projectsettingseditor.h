#ifndef PROJECTSETTINGSEDITOR_H
#define PROJECTSETTINGSEDITOR_H

#include <QMainWindow>
#include "project.h"
#include "ui_projectsettingseditor.h"

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

    static const int eventsTab;
    void setTab(int index);
    void closeQuietly();

signals:
    void reloadProject();

private:
    Ui::ProjectSettingsEditor *ui;
    Project *project;

    bool hasUnsavedChanges = false;
    bool projectNeedsReload = false;
    bool refreshing = false;
    const QString baseDir;
    QHash<QString, QString> editedPokemonIconPaths;
    QString prevIconSpecies;

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

    void createConfigTextTable(const QList<QPair<QString, QString>> configPairs, bool filesTab);
    void createProjectPathsTable();
    void createProjectIdentifiersTable();
    QString chooseProjectFile(const QString &defaultFilepath);
    void choosePrefabsFile();
    void chooseImageFile(QLineEdit * filepathEdit);
    void chooseFile(QLineEdit * filepathEdit, const QString &description, const QString &extensions);
    QString stripProjectDir(QString s);
    void disableParsedSetting(QWidget * widget, const QString &name, const QString &filepath);
    void updateMaskOverlapWarning(QLabel * warning, QList<UIntSpinBox*> masks);
    QStringList getWarpBehaviorsList();
    void setWarpBehaviorsList(QStringList list);

private slots:
    void dialogButtonClicked(QAbstractButton *button);
    void importDefaultPrefabsClicked(bool);
    void updateAttributeLimits(const QString &attrSize);
    void updatePokemonIconPath(const QString &species);
    void markEdited();
    void on_mainTabs_tabBarClicked(int index);
    void updateBlockMaskOverlapWarning();
    void updateAttributeMaskOverlapWarning();
    void updateWarpBehaviorsList(bool adding);
};

#endif // PROJECTSETTINGSEDITOR_H
