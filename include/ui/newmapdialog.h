#ifndef NEWMAPDIALOG_H
#define NEWMAPDIALOG_H

#include <QDialog>
#include <QString>
#include "editor.h"
#include "project.h"
#include "map.h"
#include "mapheaderform.h"
#include "newlayoutform.h"
#include "lib/collapsiblesection.h"

namespace Ui {
class NewMapDialog;
}

class NewMapDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NewMapDialog(QWidget *parent = nullptr, Project *project = nullptr);
    ~NewMapDialog();
    void init();
    void init(int tabIndex, QString data);
    void init(Layout *);
    void accept() override;

signals:
    void applied(const QString &newMapName);

private:
    Ui::NewMapDialog *ui;
    Project *project;
    CollapsibleSection *headerSection;
    MapHeaderForm *headerForm;
    Layout *importedLayout = nullptr;
    Project::NewMapSettings *settings = nullptr;

    // Each of these validation functions will allow empty names up until `OK` is selected,
    // because clearing the text during editing is common and we don't want to flash errors for this.
    bool validateMapID(bool allowEmpty = false);
    bool validateName(bool allowEmpty = false);
    bool validateGroup(bool allowEmpty = false);

    void saveSettings();
    bool isExistingLayout() const;
    void useLayoutSettings(const Layout *mapLayout);
    void useLayoutIdSettings(const QString &layoutId);

private slots:
    void dialogButtonClicked(QAbstractButton *button);
    void on_lineEdit_Name_textChanged(const QString &);
    void on_lineEdit_MapID_textChanged(const QString &);
    void on_comboBox_Group_currentTextChanged(const QString &text);
};

#endif // NEWMAPDIALOG_H
