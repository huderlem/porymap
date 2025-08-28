#ifndef MAPIMAGEEXPORTER_H
#define MAPIMAGEEXPORTER_H

#include "project.h"
#include "checkeredbgscene.h"

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
    CheckeredBgScene *m_scene = nullptr;
    QGifImage *m_timelapseGifImage = nullptr;
    QBuffer *m_timelapseBuffer = nullptr;
    QMovie *m_timelapseMovie = nullptr;
    QGraphicsPixmapItem *m_preview = nullptr;
    QImage m_previewImage;

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
    QImage getStitchedImage(QProgressDialog *progress);
    QImage getFormattedMapImage();
    void paintBorder(QPainter *painter, Layout *layout);
    void paintCollision(QPainter *painter, Layout *layout);
    void paintConnections(QPainter *painter, const Map *map);
    void paintEvents(QPainter *painter, const Map *map);
    void paintGrid(QPainter *painter, const Layout *layout = nullptr);
    QMargins getMargins(const Map *map);
    QImage getExpandedImage(const QImage &image, const QSize &targetSize, const QColor &fillColor);
    bool currentHistoryAppliesToFrame(QUndoStack *historyStack);

protected:
    virtual void showEvent(QShowEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;

private:
    void setShowGrid(bool checked);
    void setShowBorder(bool checked);
    void setShowObjects(bool checked);
    void setShowWarps(bool checked);
    void setShowBgs(bool checked);
    void setShowTriggers(bool checked);
    void setShowHealLocations(bool checked);
    void setShowAllEvents(bool checked);
    void setShowConnectionUp(bool checked);
    void setShowConnectionDown(bool checked);
    void setShowConnectionLeft(bool checked);
    void setShowConnectionRight(bool checked);
    void setShowAllConnections(bool checked);
    void setShowCollision(bool checked);
    void setDisablePreviewScaling(bool checked);
    void setDisablePreviewUpdates(bool checked);

    void on_pushButton_Reset_pressed();
    void on_spinBox_TimelapseDelay_editingFinished();
    void on_spinBox_FrameSkip_editingFinished();
};

#endif // MAPIMAGEEXPORTER_H
