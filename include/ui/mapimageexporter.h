#ifndef MAPIMAGEEXPORTER_H
#define MAPIMAGEEXPORTER_H

#include "project.h"

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
    explicit MapImageExporter(QWidget *parent, Project *project, Layout *layout, ImageExporterMode mode = ImageExporterMode::Normal)
                        : MapImageExporter(parent, project, nullptr, layout, mode) {};
    explicit MapImageExporter(QWidget *parent, Project *project, Map *map, ImageExporterMode mode = ImageExporterMode::Normal)
                        : MapImageExporter(parent, project, map, map->layout(), mode) {};
    ~MapImageExporter();

    ImageExporterMode mode() const { return m_mode; }

private:
    explicit MapImageExporter(QWidget *parent, Project *project, Map *map, Layout *layout, ImageExporterMode mode);

    Ui::MapImageExporter *ui;
    Project *m_project = nullptr;
    Map *m_map = nullptr;
    Layout *m_layout = nullptr;
    QGraphicsScene *m_scene = nullptr;

    QPixmap m_preview;

    ImageExporterSettings m_settings;
    ImageExporterMode m_mode = ImageExporterMode::Normal;

    QString getTitle(ImageExporterMode mode);
    QString getDescription(ImageExporterMode mode);
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
    void updateMapSelection(const QString &text);

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
