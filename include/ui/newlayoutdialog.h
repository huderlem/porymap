#ifndef NEWLAYOUTDIALOG_H
#define NEWLAYOUTDIALOG_H

#include <QDialog>
#include <QString>
#include "editor.h"
#include "project.h"
#include "map.h"
#include "mapheaderform.h"
#include "newlayoutform.h"
#include "lib/collapsiblesection.h"

namespace Ui {
class NewLayoutDialog;
}

class NewLayoutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit NewLayoutDialog(QWidget *parent = nullptr, Project *project = nullptr);
    ~NewLayoutDialog();
    void init(Layout *);
    void accept() override;

signals:
    void applied(const QString &newLayoutId);

private:
    Ui::NewLayoutDialog *ui;
    Project *project;
    Layout *importedLayout = nullptr;
    Layout::Settings *settings = nullptr;

    // Each of these validation functions will allow empty names up until `OK` is selected,
    // because clearing the text during editing is common and we don't want to flash errors for this.
    bool validateLayoutID(bool allowEmpty = false);
    bool validateName(bool allowEmpty = false);

    void saveSettings();
    bool isExistingLayout() const;
    void useLayoutSettings(Layout *mapLayout);

private slots:
    //void on_comboBox_Layout_currentTextChanged(const QString &text);//TODO
    void dialogButtonClicked(QAbstractButton *button);
    void on_lineEdit_Name_textChanged(const QString &);
    void on_lineEdit_LayoutID_textChanged(const QString &);
};

#endif // NEWLAYOUTDIALOG_H
