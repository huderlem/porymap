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
    bool disablePreviewScaling = false;
    bool disablePreviewUpdates = false;
    int timelapseSkipAmount = 1;
    int timelapseDelayMs = 200;
    // Not exposed as a setting in the UI atm (our color input widget has no alpha channel).
    QColor fillColor = Qt::transparent;
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
    void resetSettings();
    QString getTitle(ImageExporterMode mode);
    QString getDescription(ImageExporterMode mode);
    void updatePreview(bool forceUpdate = false);
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
    QPixmap getExpandedPixmap(const QPixmap &pixmap, const QSize &targetSize, const QColor &fillColor);
    bool currentHistoryAppliesToFrame(QUndoStack *historyStack);

protected:
    virtual void showEvent(QShowEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;

private:
    void setShowGrid(CheckState state);
    void setShowBorder(CheckState state);
    void setShowObjects(CheckState state);
    void setShowWarps(CheckState state);
    void setShowBgs(CheckState state);
    void setShowTriggers(CheckState state);
    void setShowHealLocations(CheckState state);
    void setShowAllEvents(CheckState state);
    void setShowConnectionUp(CheckState state);
    void setShowConnectionDown(CheckState state);
    void setShowConnectionLeft(CheckState state);
    void setShowConnectionRight(CheckState state);
    void setShowAllConnections(CheckState state);
    void setShowCollision(CheckState state);
    void setDisablePreviewScaling(CheckState state);
    void setDisablePreviewUpdates(CheckState state);

    void on_pushButton_Reset_pressed();
    void on_spinBox_TimelapseDelay_editingFinished();
    void on_spinBox_FrameSkip_editingFinished();
};

#endif // MAPIMAGEEXPORTER_H
