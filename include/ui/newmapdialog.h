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
    Map *map;
    int group;
    bool existingLayout;
    bool importedMap;
    QString layoutId;
    void init();
    void init(int tabIndex, QString data);
    void init(Layout *);
    static void setDefaultSettings(Project *project);

signals:
    void applied();

private:
    Ui::NewMapDialog *ui;
    Project *project;
    CollapsibleSection *headerSection;
    MapHeaderForm *headerForm;

    bool validateMapGroup();
    bool validateID();
    bool validateName();

    void saveSettings();
    void useLayout(QString layoutId);
    void useLayoutSettings(Layout *mapLayout);

    struct Settings {
        QString group;
        bool canFlyTo;
        NewLayoutForm::Settings layout;
        MapHeader header;
    };
    static struct Settings settings;

private slots:
    //void on_checkBox_UseExistingLayout_stateChanged(int state); //TODO
    //void on_comboBox_Layout_currentTextChanged(const QString &text);
    void on_pushButton_Accept_clicked();
    void on_lineEdit_Name_textChanged(const QString &);
    void on_lineEdit_MapID_textChanged(const QString &);
};

#endif // NEWMAPDIALOG_H
