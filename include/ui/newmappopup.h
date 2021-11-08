#ifndef NEWMAPPOPUP_H
#define NEWMAPPOPUP_H

#include <QMainWindow>
#include <QString>
#include "editor.h"
#include "project.h"
#include "map.h"

namespace Ui {
class NewMapPopup;
}

class NewMapPopup : public QMainWindow
{
    Q_OBJECT
public:
    explicit NewMapPopup(QWidget *parent = nullptr, Project *project = nullptr);
    ~NewMapPopup();
    Map *map;
    int group;
    bool existingLayout;
    bool importedMap;
    QString layoutId;
    void init(int, int, QString, QString);
    void initImportMap(MapLayout *);
    void useLayout(QString);
    void connectSignals();

signals:
    void applied();

private:
    Ui::NewMapPopup *ui;
    Project *project;
    void setDefaultValues(int, QString);
    void setDefaultValuesImportMap(MapLayout *);
    void setDefaultValuesProjectConfig(bool, MapLayout*);
    bool checkNewMapDimensions();
    bool checkNewMapGroup();

private slots:
    void on_pushButton_NewMap_Accept_clicked();
    void on_lineEdit_NewMap_Name_textChanged(const QString &);
};

#endif // NEWMAPPOPUP_H
