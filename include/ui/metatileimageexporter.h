#ifndef METATILEIMAGEEXPORTER_H
#define METATILEIMAGEEXPORTER_H

#include <QDialog>
#include <QShowEvent>
#include <QCloseEvent>
#include <QListWidget>
#include <QDropEvent>
#include <QRadioButton>

#include "config.h"
#include "checkeredbgscene.h"

class Tileset;

namespace Ui {
class MetatileImageExporter;
}

class ReorderableListWidget : public QListWidget
{
    Q_OBJECT
public:
    explicit ReorderableListWidget(QWidget *parent = nullptr) : QListWidget(parent) {
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::InternalMove);
        setDefaultDropAction(Qt::MoveAction);
    };

signals:
    void reordered();

protected:
    virtual void dropEvent(QDropEvent *event) override {
        QListWidget::dropEvent(event);
        if (event->isAccepted()) {
            emit reordered();
        }
    }
};

class MetatileImageExporter : public QDialog
{
    Q_OBJECT

public:
    struct Settings {
        OrderedMap<int,bool> layerOrder = {
            {2, true},
            {1, true},
            {0, true},
        };
        uint16_t metatileStart = 0;
        uint16_t metatileEnd = 0xFFFF;
        uint16_t numMetatilesWide = projectConfig.metatileSelectorWidth;
        bool usePrimaryTileset = true;
        bool useSecondaryTileset = false;
        bool renderPlaceholders = false;
        int transparencyMode = 0;
    };

    explicit MetatileImageExporter(QWidget *parent, Tileset *primaryTileset, Tileset *secondaryTileset, Settings *savedSettings = nullptr);
    ~MetatileImageExporter();

    bool saveImage(QString filepath = QString());
    QImage getImage();
    QString getDefaultFileName() const;
    void applySettings(const Settings &settings);
    void reset();

protected:
    virtual void showEvent(QShowEvent *) override;
    virtual void closeEvent(QCloseEvent *) override;

private:
    Ui::MetatileImageExporter *ui;

    Tileset *m_primaryTileset;
    Tileset *m_secondaryTileset;
    Settings *m_savedSettings;

    CheckeredBgScene *m_scene = nullptr;
    QGraphicsPixmapItem *m_preview = nullptr;
    QImage m_previewImage;
    bool m_previewUpdateQueued = false;
    QList<int> m_layerOrder;
    ProjectConfig m_savedConfig;
    QList<QRadioButton*> m_transparencyButtons;

    void populate(const Settings &settings);
    void updatePreview();
    void tryUpdatePreview();
    void queuePreviewUpdate();
    void tryEnforceMetatileRange();
    void syncPixelWidth();
    void syncMetatileWidth();
    void validateMetatileStart();
    void validateMetatileEnd();
    void updateMetatileRange();
    void copyRenderSettings();
    void restoreRenderSettings();
};

#endif // METATILEIMAGEEXPORTER_H
