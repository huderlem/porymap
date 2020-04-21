#ifndef MAPIMAGEEXPORTER_H
#define MAPIMAGEEXPORTER_H

#include "map.h"
#include "editor.h"

#include <QDialog>

namespace Ui {
class MapImageExporter;
}

class MapImageExporter : public QDialog
{
    Q_OBJECT

public:
    explicit MapImageExporter(QWidget *parent, Editor *editor, bool stitchMode);
    ~MapImageExporter();

private:
    Ui::MapImageExporter *ui;

    Map *map = nullptr;
    Editor *editor = nullptr;
    QGraphicsScene *scene = nullptr;

    QPixmap preview;

    bool showObjects = false;
    bool showWarps = false;
    bool showBGs = false;
    bool showTriggers = false;
    bool showHealSpots = false;
    bool showUpConnections = false;
    bool showDownConnections = false;
    bool showLeftConnections = false;
    bool showRightConnections = false;
    bool showGrid = false;
    bool showBorder = false;
    bool showCollision = false;
    bool stitchMode = false;

    void updatePreview();
    void saveImage();
    QPixmap getStitchedImage(QProgressDialog *progress, bool includeBorder);
    QPixmap getFormattedMapPixmap(Map *map, bool ignoreBorder);

private slots:
    void on_checkBox_Objects_stateChanged(int state);
    void on_checkBox_Warps_stateChanged(int state);
    void on_checkBox_BGs_stateChanged(int state);
    void on_checkBox_Triggers_stateChanged(int state);
    void on_checkBox_HealSpots_stateChanged(int state);

    void on_checkBox_ConnectionUp_stateChanged(int state);
    void on_checkBox_ConnectionDown_stateChanged(int state);
    void on_checkBox_ConnectionLeft_stateChanged(int state);
    void on_checkBox_ConnectionRight_stateChanged(int state);

    void on_checkBox_Elevation_stateChanged(int state);
    void on_checkBox_Grid_stateChanged(int state);
    void on_checkBox_Border_stateChanged(int state);

    void on_pushButton_Save_pressed();
    void on_pushButton_Reset_pressed();
    void on_pushButton_Cancel_pressed();
};

#endif // MAPIMAGEEXPORTER_H
