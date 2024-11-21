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
    explicit NewLayoutDialog(Project *project, QWidget *parent = nullptr);
    explicit NewLayoutDialog(Project *project, const Layout *layoutToCopy, QWidget *parent = nullptr);
    ~NewLayoutDialog();

    virtual void accept() override;

signals:
    void applied(const QString &newLayoutId);

private:
    Ui::NewLayoutDialog *ui;
    Project *project;
    Layout *importedLayout = nullptr;

    static Layout::Settings settings;
    static bool initializedSettings;

    // Each of these validation functions will allow empty names up until `OK` is selected,
    // because clearing the text during editing is common and we don't want to flash errors for this.
    bool validateLayoutID(bool allowEmpty = false);
    bool validateName(bool allowEmpty = false);

    void refresh();

    void saveSettings();
    bool isExistingLayout() const;

private slots:
    void dialogButtonClicked(QAbstractButton *button);
    void on_lineEdit_Name_textChanged(const QString &);
    void on_lineEdit_LayoutID_textChanged(const QString &);
};

#endif // NEWLAYOUTDIALOG_H
