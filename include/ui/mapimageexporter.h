#ifndef MAPIMAGEEXPORTER_H
#define MAPIMAGEEXPORTER_H

#include "project.h"

class QGifImage;

namespace Ui {
class MapImageExporter;
}

enum ImageExporterMode {
    Normal,
    Stitch,
    Timelapse,
};

struct ImageExporterSettings {
    QSet<Event::Group> showEvents;
    QSet<QString> showConnections;
    bool showGrid = false;
    bool showBorder = false;
    bool showCollision = false;
    bool previewActualSize = false;
    int timelapseSkipAmount = 1;
    int timelapseDelayMs = 200;
    QColor fillColor = Qt::transparent; // Not exposed as a setting in the UI atm.
};

class MapImageExporter : public QDialog
{
    Q_OBJECT

public:
    explicit MapImageExporter(QWidget *parent, Project *project, Map *map, ImageExporterMode mode = ImageExporterMode::Normal)
                        : MapImageExporter(parent, project, map, map->layout(), mode) {};
    explicit MapImageExporter(QWidget *parent, Project *project, Layout *layout, ImageExporterMode mode = ImageExporterMode::Normal)
                        : MapImageExporter(parent, project, nullptr, layout, mode) {};
    ~MapImageExporter();

    ImageExporterMode mode() const { return m_mode; }

    void setMap(Map *map);
    void setLayout(Layout *layout);

private:
    explicit MapImageExporter(QWidget *parent, Project *project, Map *map, Layout *layout, ImageExporterMode mode);

    Ui::MapImageExporter *ui;
    Project *m_project = nullptr;
    Map *m_map = nullptr;
    Layout *m_layout = nullptr;
    QGraphicsScene *m_scene = nullptr;
    QGifImage *m_timelapseGifImage = nullptr;
    QBuffer *m_timelapseBuffer = nullptr;
    QMovie *m_timelapseMovie = nullptr;
    QGraphicsPixmapItem *m_preview = nullptr;

    ImageExporterSettings m_settings;
    ImageExporterMode m_mode = ImageExporterMode::Normal;
    ImageExporterMode m_originalMode;

    void setModeSpecificUi();
    void setSelectionText(const QString &text);
    void updateMapSelection();
    QString getTitle(ImageExporterMode mode);
    QString getDescription(ImageExporterMode mode);
    void updatePreview();
    void scalePreview();
    bool eventsEnabled();
    void setEventGroupEnabled(Event::Group group, bool enable);
    bool connectionsEnabled();
    void setConnectionDirectionEnabled(const QString &dir, bool enable);
    void saveImage();
    QGifImage* createTimelapseGifImage(QProgressDialog *progress);
    QPixmap getStitchedImage(QProgressDialog *progress);
    QPixmap getFormattedMapPixmap();
    void paintBorder(QPainter *painter, Layout *layout);
    void paintCollision(QPainter *painter, Layout *layout);
    void paintConnections(QPainter *painter, const Map *map);
    void paintEvents(QPainter *painter, const Map *map);
    void paintGrid(QPainter *painter, const Layout *layout = nullptr);
    QMargins getMargins(const Map *map);
    QPixmap getExpandedPixmap(const QPixmap &pixmap, const QSize &minSize, const QColor &fillColor);
    bool currentHistoryAppliesToFrame(QUndoStack *historyStack);

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
