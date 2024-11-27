#ifndef MAPIMAGEEXPORTER_H
#define MAPIMAGEEXPORTER_H

#include "map.h"
#include "editor.h"

#include <QDialog>

namespace Ui {
class MapImageExporter;
}

enum ImageExporterMode {
    Normal,
    Stitch,
    Timelapse,
};

struct ImageExporterSettings {
    bool showObjects = false;
    bool showWarps = false;
    bool showBGs = false;
    bool showTriggers = false;
    bool showHealLocations = false;
    bool showUpConnections = false;
    bool showDownConnections = false;
    bool showLeftConnections = false;
    bool showRightConnections = false;
    bool showGrid = false;
    bool showBorder = false;
    bool showCollision = false;
    bool previewActualSize = false;
    int timelapseSkipAmount = 1;
    int timelapseDelayMs = 200;
};

class MapImageExporter : public QDialog
{
    Q_OBJECT

public:
    explicit MapImageExporter(QWidget *parent, Editor *editor, ImageExporterMode mode);
    ~MapImageExporter();

private:
    Ui::MapImageExporter *ui;

    Layout *m_layout = nullptr;
    Map *m_map = nullptr;
    Editor *m_editor = nullptr;
    QGraphicsScene *m_scene = nullptr;

    QPixmap m_preview;

    ImageExporterSettings m_settings;
    ImageExporterMode m_mode = ImageExporterMode::Normal;

    void updatePreview();
    void scalePreview();
    void updateShowBorderState();
    void saveImage();
    QPixmap getStitchedImage(QProgressDialog *progress, bool includeBorder);
    QPixmap getFormattedMapPixmap();
    QPixmap getFormattedMapPixmap(Map *map, bool ignoreBorder = false);
    QPixmap getFormattedLayoutPixmap(Layout *layout, bool ignoreBorder = false, bool ignoreGrid = false);
    void paintGrid(QPixmap *pixmap, bool ignoreBorder = false);
    bool historyItemAppliesToFrame(const QUndoCommand *command);

protected:
    virtual void showEvent(QShowEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;

private slots:
    void on_checkBox_Objects_stateChanged(int state);
    void on_checkBox_Warps_stateChanged(int state);
    void on_checkBox_BGs_stateChanged(int state);
    void on_checkBox_Triggers_stateChanged(int state);
    void on_checkBox_HealLocations_stateChanged(int state);
    void on_checkBox_AllEvents_stateChanged(int state);

    void on_checkBox_ConnectionUp_stateChanged(int state);
    void on_checkBox_ConnectionDown_stateChanged(int state);
    void on_checkBox_ConnectionLeft_stateChanged(int state);
    void on_checkBox_ConnectionRight_stateChanged(int state);
    void on_checkBox_AllConnections_stateChanged(int state);

    void on_checkBox_Elevation_stateChanged(int state);
    void on_checkBox_Grid_stateChanged(int state);
    void on_checkBox_Border_stateChanged(int state);

    void on_pushButton_Reset_pressed();
    void on_spinBox_TimelapseDelay_valueChanged(int delayMs);
    void on_spinBox_FrameSkip_valueChanged(int skip);

    void on_checkBox_ActualSize_stateChanged(int state);
};

#endif // MAPIMAGEEXPORTER_H
