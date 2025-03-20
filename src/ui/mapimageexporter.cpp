#include "mapimageexporter.h"
#include "ui_mapimageexporter.h"
#include "qgifimage.h"
#include "editcommands.h"
#include "filedialog.h"

#include <QImage>
#include <QPainter>
#include <QPoint>

#define STITCH_MODE_BORDER_DISTANCE 2

QString MapImageExporter::getTitle(ImageExporterMode mode) {
    switch (mode)
    {
        case ImageExporterMode::Normal:
            return QString("Export %1 Image").arg(m_map ? "Map" : "Layout");
        case ImageExporterMode::Stitch:
            return QStringLiteral("Export Map Stitch Image");
        case ImageExporterMode::Timelapse:
            return QString("Export %1 Timelapse Image").arg(m_map ? "Map" : "Layout");
    }
    return "";
}

QString MapImageExporter::getDescription(ImageExporterMode mode) {
    switch (mode)
    {
        case ImageExporterMode::Normal:
            return QString("Exports an image of the selected %1.").arg(m_map ? "Map" : "Layout");
        case ImageExporterMode::Stitch:
            return "Exports a combined image of all the maps connected to the selected map.";
        case ImageExporterMode::Timelapse:
            return QString("Exports a GIF of the edit history for the selected %1.").arg(m_map ? "Map" : "Layout");
    }
    return "";
}

MapImageExporter::MapImageExporter(QWidget *parent, Project *project, Map *map, Layout *layout, ImageExporterMode mode) :
    QDialog(parent),
    ui(new Ui::MapImageExporter),
    m_project(project),
    m_map(map),
    m_layout(layout),
    m_mode(mode)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    setWindowTitle(getTitle(m_mode));
    ui->label_Description->setText(getDescription(m_mode));
    ui->groupBox_Connections->setVisible(m_map && m_mode != ImageExporterMode::Stitch);
    ui->groupBox_Timelapse->setVisible(m_mode == ImageExporterMode::Timelapse);
    ui->groupBox_Events->setVisible(m_map != nullptr);

    // Initialize map selector.
    if (m_mode != ImageExporterMode::Timelapse) {
        if (m_map) {
            ui->comboBox_MapSelection->addItems(m_project->mapNames);
            ui->comboBox_MapSelection->setCurrentText(m_map->name());
            ui->label_MapSelection->setText(m_mode == ImageExporterMode::Stitch ? QStringLiteral("Starting Map") : QStringLiteral("Map"));
        } else if (m_layout) {
            ui->comboBox_MapSelection->addItems(m_project->layoutIds);
            ui->comboBox_MapSelection->setCurrentText(m_layout->id);
            ui->label_MapSelection->setText(QStringLiteral("Layout"));
        }
    } else {
        // At the moment edit history for events (and the DraggablePixmapItem class)
        // depend on the editor and assume their map is the current map.
        // Until this is resolved the selected map cannot be changed in Timelapse mode.
        ui->comboBox_MapSelection->setVisible(false);
        ui->label_MapSelection->setVisible(false);
    }
    ui->graphicsView_Preview->setFocus();

    connect(ui->pushButton_Save,   &QPushButton::pressed, this, &MapImageExporter::saveImage);
    connect(ui->pushButton_Cancel, &QPushButton::pressed, this, &MapImageExporter::close);
    connect(ui->comboBox_MapSelection, &QComboBox::currentTextChanged, this, &MapImageExporter::updateMapSelection);
}

MapImageExporter::~MapImageExporter() {
    delete m_scene;
    delete ui;
}

// Allow the window to open before displaying the preview.
void MapImageExporter::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (!event->spontaneous())
        QTimer::singleShot(0, this, &MapImageExporter::updatePreview);
}

void MapImageExporter::resizeEvent(QResizeEvent *event) {
    QDialog::resizeEvent(event);
    scalePreview();
}

void MapImageExporter::updateMapSelection(const QString &text) {
    if (m_map) {
        if (!m_project->mapNames.contains(text))
            return;
        Map *newMap = m_project->loadMap(text);
        if (newMap == m_map)
            return;
        m_map = newMap;
    } else {
        if (!m_project->layoutIds.contains(text))
            return;
        Layout *newLayout = m_project->loadLayout(text);
        if (newLayout == m_layout)
            return;
        m_layout = newLayout;
    }
    updatePreview();
}

void MapImageExporter::saveImage() {
    // Make sure preview is up-to-date before we save.
    if (m_preview.isNull())
        updatePreview();
    if (m_preview.isNull())
        return;

    const QString itemName = m_map ? m_map->name() : m_layout->name;
    QString defaultFilename;
    switch (m_mode)
    {
        case ImageExporterMode::Normal:
            defaultFilename = itemName;
            break;
        case ImageExporterMode::Stitch:
            defaultFilename = QString("Stitch_From_%1").arg(itemName);
            break;
        case ImageExporterMode::Timelapse:
            defaultFilename = QString("Timelapse_%1").arg(itemName);
            break;
    }

    QString defaultFilepath = QString("%1/%2.%3")
            .arg(FileDialog::getDirectory())
            .arg(defaultFilename)
            .arg(m_mode == ImageExporterMode::Timelapse ? "gif" : "png");
    QString filter = m_mode == ImageExporterMode::Timelapse ? "Image Files (*.gif)" : "Image Files (*.png *.jpg *.bmp)";
    QString filepath = FileDialog::getSaveFileName(this, windowTitle(), defaultFilepath, filter);
    if (!filepath.isEmpty()) {
        switch (m_mode) {
            case ImageExporterMode::Normal:
            case ImageExporterMode::Stitch:
                // Normal and Stitch modes already have the image ready to go in the preview.
                m_preview.save(filepath);
                break;
            case ImageExporterMode::Timelapse:
                // Timelapse will play in order of layout changes then map changes (events)
                // TODO: potentially update in the future?
                QGifImage timelapseImg;
                timelapseImg.setDefaultDelay(m_settings.timelapseDelayMs);
                timelapseImg.setDefaultTransparentColor(QColor(0, 0, 0));

                // lambda to avoid redundancy
                auto generateTimelapseFromHistory = [this, &timelapseImg](QString progressText, QUndoStack *historyStack){
                    QProgressDialog progress(progressText, "Cancel", 0, 1, this);
                    progress.setAutoClose(true);
                    progress.setWindowModality(Qt::WindowModal);
                    progress.setModal(true);
                    progress.setMaximum(1);
                    progress.setValue(0);

                    int maxWidth = m_layout->getWidth() * 16;
                    int maxHeight = m_layout->getHeight() * 16;
                    if (m_settings.showBorder) {
                        maxWidth += 2 * STITCH_MODE_BORDER_DISTANCE * 16;
                        maxHeight += 2 * STITCH_MODE_BORDER_DISTANCE * 16;
                    }
                    // Rewind to the specified start of the map edit history.
                    int i = 0;
                    while (historyStack->canUndo()) {
                        progress.setValue(i);
                        historyStack->undo();
                        int width = m_layout->getWidth() * 16;
                        int height = m_layout->getHeight() * 16;
                        if (m_settings.showBorder) {
                            width += 2 * STITCH_MODE_BORDER_DISTANCE * 16;
                            height += 2 * STITCH_MODE_BORDER_DISTANCE * 16;
                        }
                        if (width > maxWidth) {
                            maxWidth = width;
                        }
                        if (height > maxHeight) {
                            maxHeight = height;
                        }
                        i++;
                    }

                    // Draw each frame, skpping the specified number of map edits in
                    // the undo history.
                    progress.setMaximum(i);
                    while (i > 0) {
                        if (progress.wasCanceled()) {
                            progress.close();
                            while (i > 0 && historyStack->canRedo()) {
                                i--;
                                historyStack->redo();
                            }
                            return;
                        }
                        while (historyStack->canRedo() &&
                               !historyItemAppliesToFrame(historyStack->command(historyStack->index()))) {
                            i--;
                            historyStack->redo();
                        }
                        progress.setValue(progress.maximum() - i);
                        QPixmap pixmap = getFormattedMapPixmap();
                        if (pixmap.width() < maxWidth || pixmap.height() < maxHeight) {
                            QPixmap pixmap2 = QPixmap(maxWidth, maxHeight);
                            QPainter painter(&pixmap2);
                            pixmap2.fill(QColor(0, 0, 0));
                            painter.drawPixmap(0, 0, pixmap.width(), pixmap.height(), pixmap);
                            painter.end();
                            pixmap = pixmap2;
                        }
                        timelapseImg.addFrame(pixmap.toImage());
                        for (int j = 0; j < m_settings.timelapseSkipAmount; j++) {
                            if (i > 0) {
                                i--;
                                historyStack->redo();
                                while (historyStack->canRedo() &&
                                       !historyItemAppliesToFrame(historyStack->command(historyStack->index()))) {
                                    i--;
                                    historyStack->redo();
                                }
                            }
                        }
                    }
                    // The latest map state is the last animated frame.
                    QPixmap pixmap = getFormattedMapPixmap();
                    timelapseImg.addFrame(pixmap.toImage());
                    progress.close();
                };

                if (m_layout)
                    generateTimelapseFromHistory("Building layout timelapse...", &m_layout->editHistory);

                if (m_map)
                    generateTimelapseFromHistory("Building map timelapse...", m_map->editHistory());

                timelapseImg.save(filepath);
                break;
        }
        close();
    }
}

bool MapImageExporter::historyItemAppliesToFrame(const QUndoCommand *command) {
    if (command->isObsolete())
        return false;

    switch (command->id() & 0xFF) {
        case CommandId::ID_PaintMetatile:
        case CommandId::ID_BucketFillMetatile:
        case CommandId::ID_MagicFillMetatile:
        case CommandId::ID_ShiftMetatiles:
        case CommandId::ID_ResizeLayout:
        case CommandId::ID_ScriptEditLayout:
            return true;
        case CommandId::ID_PaintCollision:
        case CommandId::ID_BucketFillCollision:
        case CommandId::ID_MagicFillCollision:
            return m_settings.showCollision;
        case CommandId::ID_PaintBorder:
            return m_settings.showBorder;
        case CommandId::ID_MapConnectionMove:
        case CommandId::ID_MapConnectionChangeDirection:
        case CommandId::ID_MapConnectionChangeMap:
        case CommandId::ID_MapConnectionAdd:
        case CommandId::ID_MapConnectionRemove:
            return m_settings.showUpConnections || m_settings.showDownConnections || m_settings.showLeftConnections || m_settings.showRightConnections;
        case CommandId::ID_EventMove:
        case CommandId::ID_EventShift:
        case CommandId::ID_EventCreate:
        case CommandId::ID_EventDelete:
        case CommandId::ID_EventDuplicate: {
            bool eventTypeIsApplicable =
                       (m_settings.showObjects       && (command->id() & IDMask_EventType_Object)  != 0)
                    || (m_settings.showWarps         && (command->id() & IDMask_EventType_Warp)    != 0)
                    || (m_settings.showBGs           && (command->id() & IDMask_EventType_BG)      != 0)
                    || (m_settings.showTriggers      && (command->id() & IDMask_EventType_Trigger) != 0)
                    || (m_settings.showHealLocations && (command->id() & IDMask_EventType_Heal)    != 0);
            return eventTypeIsApplicable;
        }
        default:
            return false;
    }
}

struct StitchedMap {
    int x;
    int y;
    Map* map;
};

QPixmap MapImageExporter::getStitchedImage(QProgressDialog *progress, bool includeBorder) {
    // Do a breadth-first search to gather a collection of
    // all reachable maps with their relative offsets.
    QSet<QString> visited;
    QList<StitchedMap> stitchedMaps;
    QList<StitchedMap> unvisited;
    unvisited.append(StitchedMap{0, 0, m_map});

    progress->setLabelText("Gathering stitched maps...");
    while (!unvisited.isEmpty()) {
        if (progress->wasCanceled()) {
            return QPixmap();
        }
        progress->setMaximum(visited.size() + unvisited.size());
        progress->setValue(visited.size());

        StitchedMap cur = unvisited.takeFirst();
        if (visited.contains(cur.map->name()))
            continue;
        visited.insert(cur.map->name());
        stitchedMaps.append(cur);

        for (const auto &connection : cur.map->getConnections()) {
            if (!connection->isCardinal()) continue;
            QPoint pos = connection->relativePos();
            unvisited.append(StitchedMap{cur.x + pos.x(), cur.y + pos.y(), connection->targetMap()});
        }
    }

    // Determine the overall dimensions of the stitched maps.
    int maxX = INT_MIN;
    int minX = INT_MAX;
    int maxY = INT_MIN;
    int minY = INT_MAX;
    for (StitchedMap map : stitchedMaps) {
        int left = map.x;
        int right = map.x + map.map->getWidth();
        int top = map.y;
        int bottom = map.y + map.map->getHeight();
        if (left < minX)
            minX = left;
        if (right > maxX)
            maxX = right;
        if (top < minY)
            minY = top;
        if (bottom > maxY)
            maxY = bottom;
    }

    if (includeBorder) {
        minX -= STITCH_MODE_BORDER_DISTANCE;
        maxX += STITCH_MODE_BORDER_DISTANCE;
        minY -= STITCH_MODE_BORDER_DISTANCE;
        maxY += STITCH_MODE_BORDER_DISTANCE;
    }

    // Draw the maps on the full canvas, while taking
    // their respective offsets into account.
    progress->setLabelText("Drawing stitched maps...");
    progress->setValue(0);
    progress->setMaximum(stitchedMaps.size());
    int numDrawn = 0;
    QPixmap stitchedPixmap((maxX - minX) * 16, (maxY - minY) * 16);
    stitchedPixmap.fill(Qt::black);
    QPainter painter(&stitchedPixmap);
    for (StitchedMap map : stitchedMaps) {
        if (progress->wasCanceled()) {
            return QPixmap();
        }
        progress->setValue(numDrawn);
        numDrawn++;

        int pixelX = (map.x - minX) * 16;
        int pixelY = (map.y - minY) * 16;
        if (includeBorder) {
            pixelX -= STITCH_MODE_BORDER_DISTANCE * 16;
            pixelY -= STITCH_MODE_BORDER_DISTANCE * 16;
        }
        QPixmap pixmap = getFormattedMapPixmap(map.map);
        painter.drawPixmap(pixelX, pixelY, pixmap);
    }

    // When including the borders, we simply draw all the maps again
    // without their borders, since the first pass results in maps
    // being occluded by other map borders.
    if (includeBorder) {
        progress->setLabelText("Drawing stitched maps without borders...");
        progress->setValue(0);
        progress->setMaximum(stitchedMaps.size());
        numDrawn = 0;
        for (StitchedMap map : stitchedMaps) {
            if (progress->wasCanceled()) {
                return QPixmap();
            }
            progress->setValue(numDrawn);
            numDrawn++;

            int pixelX = (map.x - minX) * 16;
            int pixelY = (map.y - minY) * 16;
            QPixmap pixmapWithoutBorders = getFormattedMapPixmap(map.map, true);
            painter.drawPixmap(pixelX, pixelY, pixmapWithoutBorders);
        }
    }

    return stitchedPixmap;
}

void MapImageExporter::updatePreview() {
    if (m_scene) {
        delete m_scene;
        m_scene = nullptr;
    }
    m_scene = new QGraphicsScene;

    if (m_mode == ImageExporterMode::Stitch) { 
        QProgressDialog progress("Building map stitch...", "Cancel", 0, 1, this);
        progress.setAutoClose(true);
        progress.setWindowModality(Qt::WindowModal);
        progress.setModal(true);
        progress.setMinimumDuration(1000);
        m_preview = getStitchedImage(&progress, m_settings.showBorder);
        progress.close();
    } else {
        // Timelapse mode doesn't currently have a real preview. It just displays the current map as in Normal mode.
        m_preview = getFormattedMapPixmap();
    }
    m_scene->addPixmap(m_preview);
    ui->graphicsView_Preview->setScene(m_scene);
    scalePreview();
}

void MapImageExporter::scalePreview() {
    if (m_scene && !m_settings.previewActualSize){
       ui->graphicsView_Preview->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatioByExpanding);
    }
}

QPixmap MapImageExporter::getFormattedMapPixmap() {
    return m_map ? getFormattedMapPixmap(m_map) : getFormattedLayoutPixmap(m_layout);
}

QPixmap MapImageExporter::getFormattedLayoutPixmap(Layout *layout, bool ignoreBorder, bool ignoreGrid) {
    if (!layout)
        return QPixmap();

    layout->render(true);
    QPixmap pixmap = layout->pixmap;

    if (m_settings.showCollision) {
        QPainter collisionPainter(&pixmap);
        layout->renderCollision(true);
        collisionPainter.setOpacity(static_cast<qreal>(porymapConfig.collisionOpacity) / 100);
        collisionPainter.drawPixmap(0, 0, layout->collision_pixmap);
        collisionPainter.end();
    }

    // draw map border
    // note: this will break when allowing map to be selected from drop down maybe
    int borderHeight = 0, borderWidth = 0;
    if (!ignoreBorder && m_settings.showBorder) {
        int borderDistance = m_mode ? STITCH_MODE_BORDER_DISTANCE : BORDER_DISTANCE;
        layout->renderBorder();
        int borderHorzDist = layout->getBorderDrawWidth();
        int borderVertDist = layout->getBorderDrawHeight();
        borderWidth = borderDistance * 16;
        borderHeight = borderDistance * 16;
        QPixmap newPixmap = QPixmap(layout->pixmap.width() + borderWidth * 2, layout->pixmap.height() + borderHeight * 2);
        QPainter borderPainter(&newPixmap);
        for (int y = borderDistance - borderVertDist; y < layout->getHeight() + borderVertDist * 2; y += layout->getBorderHeight()) {
            for (int x = borderDistance - borderHorzDist; x < layout->getWidth() + borderHorzDist * 2; x += layout->getBorderWidth()) {
                borderPainter.drawPixmap(x * 16, y * 16, layout->border_pixmap);
            }
        }
        borderPainter.drawImage(borderWidth, borderHeight, pixmap.toImage());
        borderPainter.end();
        pixmap = newPixmap;
    }

    // The grid should be painted last, so if this layout pixmap is being painted
    // as part of a map (which has more to paint after this) then don't paint the grid yet.
    if (!ignoreGrid)
        paintGrid(&pixmap, ignoreBorder);

    return pixmap;
}

QPixmap MapImageExporter::getFormattedMapPixmap(Map *map, bool ignoreBorder) {
    if (!map)
        return QPixmap();

    QPixmap pixmap = getFormattedLayoutPixmap(map->layout(), ignoreBorder, true);

    if (!ignoreBorder && (m_settings.showUpConnections || m_settings.showDownConnections || m_settings.showLeftConnections || m_settings.showRightConnections)) {
        // if showing connections, draw on outside of image
        QPainter connectionPainter(&pixmap);
        
        int borderDistance = m_mode ? STITCH_MODE_BORDER_DISTANCE : BORDER_DISTANCE;
        for (const auto &connection : m_map->getConnections()) {
            const QString direction = connection->direction();
            if (direction == "up") {
                if (!m_settings.showUpConnections) continue;
            } else if (direction == "down") {
                if (!m_settings.showDownConnections) continue;
            } else if (direction == "left") {
                if (!m_settings.showLeftConnections) continue;
            } else if (direction == "right") {
                if (!m_settings.showRightConnections) continue;
            } else continue; // Ignore any other directions

            QPoint pos = connection->relativePos(true);
            connectionPainter.drawImage((pos.x() + borderDistance) * 16, (pos.y() + borderDistance) * 16, connection->render().toImage());
        }
        connectionPainter.end();
    }

    // draw events
    if (m_settings.showObjects || m_settings.showWarps || m_settings.showBGs || m_settings.showTriggers || m_settings.showHealLocations) {
        QPainter eventPainter(&pixmap);
        int pixelOffset = 0;
        if (!ignoreBorder && m_settings.showBorder) {
            pixelOffset = m_mode == ImageExporterMode::Normal ? BORDER_DISTANCE * 16 : STITCH_MODE_BORDER_DISTANCE * 16;
        }
        const QList<Event *> events = map->getEvents();
        for (const auto &event : events) {
            Event::Group group = event->getEventGroup();
            if ((m_settings.showObjects && group == Event::Group::Object)
             || (m_settings.showWarps && group == Event::Group::Warp)
             || (m_settings.showBGs && group == Event::Group::Bg)
             || (m_settings.showTriggers && group == Event::Group::Coord)
             || (m_settings.showHealLocations && group == Event::Group::Heal)) {
                m_project->loadEventPixmap(event);
                eventPainter.setOpacity(event->getUsesDefaultPixmap() ? 0.7 : 1.0);
                eventPainter.drawImage(QPoint(event->getPixelX() + pixelOffset, event->getPixelY() + pixelOffset), event->getPixmap().toImage());
            }
        }
        eventPainter.end();
    }
    paintGrid(&pixmap, ignoreBorder);

    return pixmap;
}

void MapImageExporter::paintGrid(QPixmap *pixmap, bool ignoreBorder) {
    // draw grid directly onto the pixmap
    // since the last grid lines are outside of the pixmap, add a pixel to the bottom and right
    if (m_settings.showGrid) {
        bool hasBorder = !ignoreBorder && m_settings.showBorder;
        int addX = 1, addY = 1;
        if (hasBorder) addY = 0;
        if (hasBorder) addX = 0;

        QPixmap newPixmap= QPixmap(pixmap->width() + addX, pixmap->height() + addY);
        QPainter gridPainter(&newPixmap);
        gridPainter.drawImage(QPoint(0, 0), pixmap->toImage());
        for (int x = 0; x < newPixmap.width(); x += 16) {
            gridPainter.drawLine(x, 0, x, newPixmap.height());
        }
        for (int y = 0; y < newPixmap.height(); y += 16) {
            gridPainter.drawLine(0, y, newPixmap.width(), y);
        }
        gridPainter.end();
        *pixmap = newPixmap;
    }
}

void MapImageExporter::updateShowBorderState() {
    // If any of the Connections settings are enabled then this setting is locked (it's implicitly enabled)
    bool on = (m_settings.showUpConnections || m_settings.showDownConnections || m_settings.showLeftConnections || m_settings.showRightConnections);
    const QSignalBlocker blocker(ui->checkBox_Border);
    ui->checkBox_Border->setChecked(on);
    ui->checkBox_Border->setDisabled(on);
    m_settings.showBorder = on;
}

void MapImageExporter::on_checkBox_Elevation_stateChanged(int state) {
    m_settings.showCollision = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Grid_stateChanged(int state) {
    m_settings.showGrid = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Border_stateChanged(int state) {
    m_settings.showBorder = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Objects_stateChanged(int state) {
    m_settings.showObjects = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Warps_stateChanged(int state) {
    m_settings.showWarps = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_BGs_stateChanged(int state) {
    m_settings.showBGs = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Triggers_stateChanged(int state) {
    m_settings.showTriggers = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_HealLocations_stateChanged(int state) {
    m_settings.showHealLocations = (state == Qt::Checked);
    updatePreview();
}

// Shortcut setting for enabling all events
void MapImageExporter::on_checkBox_AllEvents_stateChanged(int state) {
    bool on = (state == Qt::Checked);

    const QSignalBlocker b_Objects(ui->checkBox_Objects);
    ui->checkBox_Objects->setChecked(on);
    ui->checkBox_Objects->setDisabled(on);
    m_settings.showObjects = on;

    const QSignalBlocker b_Warps(ui->checkBox_Warps);
    ui->checkBox_Warps->setChecked(on);
    ui->checkBox_Warps->setDisabled(on);
    m_settings.showWarps = on;

    const QSignalBlocker b_BGs(ui->checkBox_BGs);
    ui->checkBox_BGs->setChecked(on);
    ui->checkBox_BGs->setDisabled(on);
    m_settings.showBGs = on;

    const QSignalBlocker b_Triggers(ui->checkBox_Triggers);
    ui->checkBox_Triggers->setChecked(on);
    ui->checkBox_Triggers->setDisabled(on);
    m_settings.showTriggers = on;

    const QSignalBlocker b_HealLocations(ui->checkBox_HealLocations);
    ui->checkBox_HealLocations->setChecked(on);
    ui->checkBox_HealLocations->setDisabled(on);
    m_settings.showHealLocations = on;

    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionUp_stateChanged(int state) {
    m_settings.showUpConnections = (state == Qt::Checked);
    updateShowBorderState();
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionDown_stateChanged(int state) {
    m_settings.showDownConnections = (state == Qt::Checked);
    updateShowBorderState();
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionLeft_stateChanged(int state) {
    m_settings.showLeftConnections = (state == Qt::Checked);
    updateShowBorderState();
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionRight_stateChanged(int state) {
    m_settings.showRightConnections = (state == Qt::Checked);
    updateShowBorderState();
    updatePreview();
}

// Shortcut setting for enabling all connection directions
void MapImageExporter::on_checkBox_AllConnections_stateChanged(int state) {
    bool on = (state == Qt::Checked);

    const QSignalBlocker b_Up(ui->checkBox_ConnectionUp);
    ui->checkBox_ConnectionUp->setChecked(on);
    ui->checkBox_ConnectionUp->setDisabled(on);
    m_settings.showUpConnections = on;

    const QSignalBlocker b_Down(ui->checkBox_ConnectionDown);
    ui->checkBox_ConnectionDown->setChecked(on);
    ui->checkBox_ConnectionDown->setDisabled(on);
    m_settings.showDownConnections = on;

    const QSignalBlocker b_Left(ui->checkBox_ConnectionLeft);
    ui->checkBox_ConnectionLeft->setChecked(on);
    ui->checkBox_ConnectionLeft->setDisabled(on);
    m_settings.showLeftConnections = on;

    const QSignalBlocker b_Right(ui->checkBox_ConnectionRight);
    ui->checkBox_ConnectionRight->setChecked(on);
    ui->checkBox_ConnectionRight->setDisabled(on);
    m_settings.showRightConnections = on;

    updateShowBorderState();
    updatePreview();
}

void MapImageExporter::on_checkBox_ActualSize_stateChanged(int state) {
    m_settings.previewActualSize = (state == Qt::Checked);
    if (m_settings.previewActualSize) {
        ui->graphicsView_Preview->resetTransform();
    } else {
        scalePreview();
    }
}

void MapImageExporter::on_pushButton_Reset_pressed() {
    m_settings = {};
    for (auto widget : this->findChildren<QCheckBox *>()) {
        const QSignalBlocker b(widget); // Prevent calls to updatePreview
        widget->setChecked(false);
    }
    ui->spinBox_TimelapseDelay->setValue(m_settings.timelapseDelayMs);
    ui->spinBox_FrameSkip->setValue(m_settings.timelapseSkipAmount);
    updatePreview();
}

void MapImageExporter::on_spinBox_TimelapseDelay_valueChanged(int delayMs) {
    m_settings.timelapseDelayMs = delayMs;
}

void MapImageExporter::on_spinBox_FrameSkip_valueChanged(int skip) {
    m_settings.timelapseSkipAmount = skip;
}
