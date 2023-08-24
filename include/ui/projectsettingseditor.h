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
    void saved();

private:
    Ui::ProjectSettingsEditor *ui;
    Project *project;
    NoScrollComboBox *combo_defaultPrimaryTileset;
    NoScrollComboBox *combo_defaultSecondaryTileset;
    NoScrollComboBox *combo_baseGameVersion;
    NoScrollComboBox *combo_attributesSize;

    bool hasUnsavedChanges = false;

    void initUi();
    void saveFields();
    void connectSignals();
    void refresh();
    bool prompt(const QString &text);

private slots:
    void dialogButtonClicked(QAbstractButton *button);
    void markEdited();
};

#endif // PROJECTSETTINGSEDITOR_H
