#ifndef REGIONMAPPROPERTIESDIALOG_H
#define REGIONMAPPROPERTIESDIALOG_H

#include "orderedjson.h"

#include <QDialog>
#include <QFileDialog>

class Project;

namespace Ui {
class RegionMapPropertiesDialog;
}

class RegionMapPropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegionMapPropertiesDialog(QWidget *parent = nullptr);
    ~RegionMapPropertiesDialog();

    void setProject(Project *project) { this->project = project; }

    void setProperties(poryjson::Json object);
    poryjson::Json saveToJson();

    virtual void accept() override;

private:
    Ui::RegionMapPropertiesDialog *ui;
    Project *project = nullptr;

    void hideMessages();

    QString browse(QString filter, QFileDialog::FileMode mode);

private slots:
    void on_browse_tilesetImagePath_clicked();
    void on_browse_tilemapBinPath_clicked();
    void on_browse_tilemapPalettePath_clicked();
    void on_browse_layoutPath_clicked();

    //void on_buttonBox_accepted();
};

#endif // REGIONMAPPROPERTIESDIALOG_H
