#include "mapimageexporter.h"
#include "ui_mapimageexporter.h"
#include "qgifimage.h"
#include "editcommands.h"
#include "filedialog.h"

#include <QImage>
#include <QPainter>
#include <QPoint>

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
    m_mode(mode),
    m_originalMode(mode)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    m_scene = new QGraphicsScene(this);
    m_preview = m_scene->addPixmap(QPixmap());
    ui->graphicsView_Preview->setScene(m_scene);

    setModeSpecificUi();

    connect(ui->pushButton_Save,   &QPushButton::pressed, this, &MapImageExporter::saveImage);
    connect(ui->pushButton_Cancel, &QPushButton::pressed, this, &MapImageExporter::close);

    // Update the map selector when the text changes.
    // We don't use QComboBox::currentTextChanged to avoid unnecessary re-rendering.
    connect(ui->comboBox_MapSelection, &QComboBox::currentIndexChanged, this, &MapImageExporter::updateMapSelection);
    connect(ui->comboBox_MapSelection->lineEdit(), &QLineEdit::editingFinished, this, &MapImageExporter::updateMapSelection);

    ui->graphicsView_Preview->setFocus();
}

MapImageExporter::~MapImageExporter() {
    delete m_timelapseGifImage;
    delete ui;
}

void MapImageExporter::setModeSpecificUi() {
    m_mode = m_originalMode;
    if (!m_map && m_mode == ImageExporterMode::Stitch) {
        // Stitch mode is not valid with only a layout open.
        // This could happen if a user opens the stitch exporter with a map, then opens a layout.
        m_mode = ImageExporterMode::Normal;
    }

    setWindowTitle(getTitle(m_mode));
    ui->label_Description->setText(getDescription(m_mode));
    ui->groupBox_Connections->setVisible(m_map && m_mode != ImageExporterMode::Stitch);
    ui->groupBox_Timelapse->setVisible(m_mode == ImageExporterMode::Timelapse);
    ui->groupBox_Events->setVisible(m_map != nullptr);

    // Initialize map selector
    const QSignalBlocker b(ui->comboBox_MapSelection);
    ui->comboBox_MapSelection->clear();
    if (m_map) {
        ui->comboBox_MapSelection->addItems(m_project->mapNames);
        ui->comboBox_MapSelection->setCurrentText(m_map->name());
        ui->label_MapSelection->setText(m_mode == ImageExporterMode::Stitch ? QStringLiteral("Starting Map") : QStringLiteral("Map"));
    } else if (m_layout) {
        ui->comboBox_MapSelection->addItems(m_project->layoutIds);
        ui->comboBox_MapSelection->setCurrentText(m_layout->id);
        ui->label_MapSelection->setText(QStringLiteral("Layout"));
    }

    if (m_mode == ImageExporterMode::Timelapse) {
        // At the moment edit history for events (and the DraggablePixmapItem class)
        // depend on the editor and assume their map is the current map.
        // Until this is resolved, the selected map and the editor's map must remain the same.
        ui->comboBox_MapSelection->setEnabled(false);
        ui->label_MapSelection->setEnabled(false);

        // Timelapse gif has artifacts with transparency, make sure it's disabled.
        m_settings.fillColor.setAlpha(255);
    }
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

void MapImageExporter::setMap(Map *map) {
    if (map) setSelectionText(map->name());
}

void MapImageExporter::setLayout(Layout *layout) {
    if (layout) setSelectionText(layout->id);
}

void MapImageExporter::setSelectionText(const QString &text) {
    const QSignalBlocker b(ui->comboBox_MapSelection);
    ui->comboBox_MapSelection->setCurrentText(text);
    updateMapSelection();
}

void MapImageExporter::updateMapSelection() {
    auto oldMap = m_map;
    auto oldLayout = m_layout;

    const QString text = ui->comboBox_MapSelection->currentText();
    if (m_project->mapNames.contains(text)) {
        auto newMap = m_project->loadMap(text);
        if (newMap) {
            m_map = newMap;
            m_layout = newMap->layout();
        }
    } else if (m_project->layoutIds.contains(text)) {
        auto newLayout = m_project->loadLayout(text);
        if (newLayout) {
            m_map = nullptr;
            m_layout = newLayout;
        }
    }

    // Ensure text in the combo box remains valid
    const QSignalBlocker b(ui->comboBox_MapSelection);
    ui->comboBox_MapSelection->setCurrentText(m_map ? m_map->name() : m_layout->id);

    if (m_map != oldMap && (!m_map || !oldMap)) {
        // Switching to or from layout-only mode
        setModeSpecificUi();
    }
    if (m_map != oldMap || m_layout != oldLayout){
        updatePreview();
    }
}

void MapImageExporter::saveImage() {
    // If the preview is empty it's because progress was canceled.
    // Try again to create it, and if it's canceled again we'll stop the export.
    if (m_preview->pixmap().isNull()) {
        updatePreview();
        if (m_preview->pixmap().isNull())
            return;
    }
    if (m_mode == ImageExporterMode::Timelapse && !m_timelapseGifImage) {
        // Shouldn't happen. We have a preview for the timelapse, but no timelapse image.
        return;
    }

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
                m_preview->pixmap().save(filepath);
                break;
            case ImageExporterMode::Timelapse:
                m_timelapseGifImage->save(filepath);
                break;
        }
        close();
    }
}

bool MapImageExporter::currentHistoryAppliesToFrame(QUndoStack *historyStack) {
    const QUndoCommand *command = historyStack->command(historyStack->index());
    if (!command || command->isObsolete())
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
            return connectionsEnabled();
        case CommandId::ID_EventMove:
        case CommandId::ID_EventShift:
        case CommandId::ID_EventCreate:
        case CommandId::ID_EventPaste:
        case CommandId::ID_EventDelete:
        case CommandId::ID_EventDuplicate: {
            if (command->id() & IDMask_EventType_Object)  return m_settings.showEvents.contains(Event::Group::Object);
            if (command->id() & IDMask_EventType_Warp)    return m_settings.showEvents.contains(Event::Group::Warp);
            if (command->id() & IDMask_EventType_BG)      return m_settings.showEvents.contains(Event::Group::Bg);
            if (command->id() & IDMask_EventType_Trigger) return m_settings.showEvents.contains(Event::Group::Coord);
            if (command->id() & IDMask_EventType_Heal)    return m_settings.showEvents.contains(Event::Group::Heal);
            return false;
        }
        default:
            return false;
    }
}

QPixmap MapImageExporter::getExpandedPixmap(const QPixmap &pixmap, const QSize &minSize, const QColor &fillColor) {
    if (pixmap.width() >= minSize.width() && pixmap.height() >= minSize.height())
        return pixmap;

    QPixmap resizedPixmap = QPixmap(minSize);
    QPainter painter(&resizedPixmap);
    resizedPixmap.fill(fillColor);
    painter.drawPixmap(0, 0, pixmap.width(), pixmap.height(), pixmap);
    painter.end();
    return resizedPixmap;
}

struct TimelapseStep {
    QUndoStack* historyStack;
    int initialStackIndex;
    QString name;
};

QGifImage* MapImageExporter::createTimelapseGifImage(QProgressDialog *progress) {
    // TODO: Timelapse will play in order of layout changes then map changes (events, connections). Potentially update in the future?
    QList<TimelapseStep> steps;
    if (m_layout) {
        steps.append({
            .historyStack = &m_layout->editHistory,
            .initialStackIndex = m_layout->editHistory.index(),
            .name = "layout",
        });
    }
    if (m_map) {
        steps.append({
            .historyStack = m_map->editHistory(),
            .initialStackIndex = m_map->editHistory()->index(),
            .name = "map",
        });
    }

    // Rewind the edit histories and get the maximum map size for the gif's canvas.
    QSize canvasSize = QSize(0,0);
    for (const auto &step : steps) {
        progress->setLabelText(QString("Rewinding %1 edit history...").arg(step.name));
        progress->setMinimum(0);
        progress->setMaximum(step.initialStackIndex);
        progress->setValue(progress->minimum());
        do {
            if (currentHistoryAppliesToFrame(step.historyStack)) {
                // This command is relevant, record the size of the map at this point.
                QMargins margins = getMargins(m_map);
                canvasSize = canvasSize.expandedTo(QSize(m_layout->getWidth() * 16 + margins.left() + margins.right(),
                                                         m_layout->getHeight() * 16 + margins.top() + margins.bottom()));
            }
            if (step.historyStack->canUndo()){
                step.historyStack->undo();
            } else break;
            progress->setValue(step.initialStackIndex - step.historyStack->index());
        } while (!progress->wasCanceled());
    }

    auto timelapseImg = new QGifImage(canvasSize);
    timelapseImg->setDefaultDelay(m_settings.timelapseDelayMs);
    timelapseImg->setDefaultTransparentColor(m_settings.fillColor);

    // Create the timelapse image frames
    for (const auto &step : steps) {
        if (step.historyStack->index() >= step.initialStackIndex)
            continue;

        // Progress is represented by the number of commands we need to redo to finish the timelapse,
        // which can be different than the number of image frames we need to create.
        progress->setLabelText(QString("Building %1 timelapse...").arg(step.name));
        progress->setMinimum(step.historyStack->index());
        progress->setMaximum(step.initialStackIndex - step.historyStack->index());
        progress->setValue(progress->minimum());

        int framesToSkip = m_settings.timelapseSkipAmount - 1;
        while (step.historyStack->canRedo() && step.historyStack->index() < step.initialStackIndex && !progress->wasCanceled()) {
            if (currentHistoryAppliesToFrame(step.historyStack) && --framesToSkip <= 0) {
                // Render frame, increasing its size if necessary to match the canvas.
                QPixmap pixmap = getExpandedPixmap(getFormattedMapPixmap(), canvasSize, m_settings.fillColor);
                timelapseImg->addFrame(pixmap.toImage());
                framesToSkip = m_settings.timelapseSkipAmount - 1;
            }
            step.historyStack->redo();
            progress->setValue(step.historyStack->index() - progress->minimum());
        }
    }

    // Ensure all edit histories are restored to their original states.
    // We already make sure above that we don't overshoot the initial state,
    // so this should only need to happen if progress was canceled.
    // Restoring the edit history is required, so we will disable canceling from here on.
    if (progress->wasCanceled()) {
        delete timelapseImg;
        timelapseImg = nullptr;
    }
    progress->setCancelButton(nullptr);
    for (const auto &step : steps) {
        if (step.historyStack->index() >= step.initialStackIndex)
            continue;
        progress->setLabelText(QString("Restoring %1 edit history...").arg(step.name));
        progress->setMinimum(0);
        progress->setMaximum(step.initialStackIndex);
        progress->setValue(step.historyStack->index());
        while (step.historyStack->canRedo() && step.historyStack->index() < step.initialStackIndex) {
            step.historyStack->redo();
            progress->setValue(step.historyStack->index());
        }
    }

    // Final frame should always be the current state of the map.
    if (timelapseImg) {
        QPixmap finalFrame = getExpandedPixmap(getFormattedMapPixmap(), canvasSize, m_settings.fillColor);
        timelapseImg->addFrame(finalFrame.toImage());
    }
    return timelapseImg;
}

struct StitchedMap {
    int x;
    int y;
    Map* map;
};

QPixmap MapImageExporter::getStitchedImage(QProgressDialog *progress) {
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
            Map *connectedMap = connection->targetMap();
            if (!connectedMap) continue;
            QPoint pos = connection->relativePos();
            unvisited.append(StitchedMap{cur.x + (pos.x() * 16), cur.y + (pos.y() * 16), connectedMap});
        }
    }
    if (stitchedMaps.isEmpty())
        return QPixmap();

    progress->setMaximum(stitchedMaps.size());
    int numDrawn = 0;

    // Determine the overall dimensions of the stitched maps.
    QRect dimensions = QRect(0, 0, m_map->getWidth(), m_map->getHeight()) + getMargins(m_map);
    for (const StitchedMap &map : stitchedMaps) {
        dimensions |= (QRect(map.x, map.y, map.map->getWidth() * 16, map.map->getHeight() * 16) + getMargins(map.map));
    }

    QPixmap stitchedPixmap(dimensions.width(), dimensions.height());
    stitchedPixmap.fill(m_settings.fillColor);

    QPainter painter(&stitchedPixmap);
    painter.translate(-dimensions.left(), -dimensions.top());

    // Borders can occlude neighboring maps, so we draw all the borders before drawing any maps.
    // Note: Borders can also overlap the borders of neighboring maps. It's not technically wrong to do this,
    //       but it might suggest to users that something is visible in-game that actually isn't.
    //       (e.g. in FRLG, Route 18's water border can overlap Fuchsia's tree border. It suggests you could
    //        see a jarring transition in-game from one of these maps, but because of the collision map the
    //        player isn't actually able to get close enough to this transition to see it).
    //       Perhaps some future export setting could limit the border rendering to the visibility range from walkable areas.
    if (m_settings.showBorder) {
        progress->setLabelText("Drawing borders...");
        progress->setValue(0);
        numDrawn = 0;
        for (const StitchedMap &map : stitchedMaps) {
            if (progress->wasCanceled()) {
                return QPixmap();
            }
            painter.translate(map.x, map.y);
            paintBorder(&painter, map.map->layout());
            painter.translate(-map.x, -map.y);

            progress->setValue(numDrawn++);
        }
    }

    // Draw the layout and collision images.
    progress->setLabelText("Drawing maps...");
    progress->setValue(0);
    numDrawn = 0;
    for (const StitchedMap &map : stitchedMaps) {
        if (progress->wasCanceled()) {
            return QPixmap();
        }
        painter.translate(map.x, map.y);
        painter.drawPixmap(0, 0, map.map->layout()->render(true));
        paintCollision(&painter, map.map->layout());
        painter.translate(-map.x, -map.y);

        progress->setValue(numDrawn++);
    }

    // Events can be occluded by neighboring maps if they are positioned
    // near or outside the map's edge, so we draw them after all the maps.
    // Nothing should be on top of the grid, so it's drawn last.
    if (m_settings.showGrid || eventsEnabled()) {
        progress->setLabelText("Drawing map decorations...");
        progress->setValue(0);
        numDrawn = 0;
        for (const StitchedMap &map : stitchedMaps) {
            if (progress->wasCanceled()) {
                return QPixmap();
            }
            painter.translate(map.x, map.y);
            paintEvents(&painter, map.map);
            paintGrid(&painter, map.map->layout());
            painter.translate(-map.x, -map.y);

            progress->setValue(numDrawn++);
        }
    }

    return stitchedPixmap;
}

void MapImageExporter::updatePreview() {
    QProgressDialog progress("", "Cancel", 0, 1, this);
    progress.setAutoClose(true);
    progress.setWindowModality(Qt::WindowModal);
    progress.setModal(true);
    progress.setMinimumDuration(1000);

    QPixmap previewPixmap;
    if (m_mode == ImageExporterMode::Normal) {
        previewPixmap = getFormattedMapPixmap();
    } else if (m_mode == ImageExporterMode::Stitch) {
        previewPixmap = getStitchedImage(&progress);
    } else if (m_mode == ImageExporterMode::Timelapse) {
        if (m_timelapseMovie)
            m_timelapseMovie->stop();

        m_timelapseGifImage = createTimelapseGifImage(&progress);
        if (!m_timelapseGifImage) {
            previewPixmap = QPixmap();
        } else {
            // We want to convert the QGifImage data into a QMovie for the preview display.
            // Both support input/output with a QIODevice, so we use a QBuffer to translate the data.
            delete m_timelapseBuffer;
            m_timelapseBuffer = new QBuffer(this);
            m_timelapseBuffer->open(QBuffer::ReadWrite);
            m_timelapseGifImage->save(m_timelapseBuffer);
            m_timelapseBuffer->close();

            delete m_timelapseMovie;
            m_timelapseMovie = new QMovie(m_timelapseBuffer, "gif", this);
            m_timelapseMovie->setCacheMode(QMovie::CacheAll);
            connect(m_timelapseMovie, &QMovie::frameChanged, [this](int) {
                m_preview->setPixmap(m_timelapseMovie->currentPixmap());
            });
            m_timelapseMovie->start();
            previewPixmap = m_timelapseMovie->currentPixmap();
        }
    }
    progress.close();

    m_preview->setPixmap(previewPixmap);
    m_scene->setSceneRect(m_scene->itemsBoundingRect());
    scalePreview();
}

void MapImageExporter::scalePreview() {
    if (!m_preview || m_settings.previewActualSize)
        return;
    ui->graphicsView_Preview->fitInView(m_preview, Qt::KeepAspectRatioByExpanding);
}

QPixmap MapImageExporter::getFormattedMapPixmap() {
    if (!m_layout)
        return QPixmap();

    m_layout->render(true);

    // Create pixmap large enough to contain the map and the marginal elements (the border, grid, etc.)
    QMargins margins = getMargins(m_map);
    QPixmap pixmap = QPixmap(m_layout->pixmap.width() + margins.left() + margins.right(),
                             m_layout->pixmap.height() + margins.top() + margins.bottom());
    pixmap.fill(m_settings.fillColor);

    QPainter painter(&pixmap);
    painter.translate(margins.left(), margins.top());

    paintBorder(&painter, m_layout);
    painter.drawPixmap(0, 0, m_layout->pixmap);
    paintCollision(&painter, m_layout);
    if (m_map) {
        paintConnections(&painter, m_map);
        paintEvents(&painter, m_map);
    }
    paintGrid(&painter, m_layout);

    return pixmap;
}

QMargins MapImageExporter::getMargins(const Map *map) {
    QMargins margins;
    if (m_settings.showBorder) {
        // The border may technically extend beyond BORDER_DISTANCE, but when the border is painted
        // we will be limiting it to the visible sight range.
        margins = QMargins(BORDER_DISTANCE, BORDER_DISTANCE, BORDER_DISTANCE, BORDER_DISTANCE) * 16;
    } else if (map && connectionsEnabled()) {
        for (const auto &connection : map->getConnections()) {
            const QString dir = connection->direction();
            if (!m_settings.showConnections.contains(dir))
                continue;
            auto targetMap = connection->targetMap();
            if (!targetMap) continue;

            QRect rect = targetMap->getConnectionRect(dir);
            if (dir == "up") margins.setTop(rect.height() * 16);
            else if (dir == "down") margins.setBottom(rect.height() * 16);
            else if (dir == "left") margins.setLeft(rect.width() * 16);
            else if (dir == "right") margins.setRight(rect.width() * 16);
        }
    }
    if (m_settings.showGrid) {
        // Account for outer grid line
        if (margins.right() == 0) margins.setRight(1);
        if (margins.bottom() == 0) margins.setBottom(1);
    }
    return margins;
}

void MapImageExporter::paintCollision(QPainter *painter, Layout *layout) {
    if (!m_settings.showCollision)
        return;

    auto savedOpacity = painter->opacity();
    painter->setOpacity(static_cast<qreal>(porymapConfig.collisionOpacity) / 100);
    painter->drawPixmap(0, 0, layout->renderCollision(true));
    painter->setOpacity(savedOpacity);
}

void MapImageExporter::paintBorder(QPainter *painter, Layout *layout) {
    if (!m_settings.showBorder)
        return;

    layout->renderBorder(true);

    // Clip parts of the border that would be beyond player visibility.
    QRect visibleArea(0, 0, layout->getWidth() * 16, layout->getHeight() * 16);
    visibleArea += (QMargins(BORDER_DISTANCE, BORDER_DISTANCE, BORDER_DISTANCE, BORDER_DISTANCE) * 16);
    painter->save();
    painter->setClipRect(visibleArea);

    int borderHorzDist = layout->getBorderDrawWidth();
    int borderVertDist = layout->getBorderDrawHeight();
    for (int y = -borderVertDist; y < layout->getHeight() + borderVertDist; y += layout->getBorderHeight())
    for (int x = -borderHorzDist; x < layout->getWidth() + borderHorzDist; x += layout->getBorderWidth()) {
         // Skip border painting if it would be fully covered by the rest of the map
        if (layout->isWithinBounds(QRect(x, y, layout->getBorderWidth(), layout->getBorderHeight())))
            continue;
        painter->drawPixmap(x * 16, y * 16, layout->border_pixmap);
    }

    painter->restore();
}

void MapImageExporter::paintConnections(QPainter *painter, const Map *map) {
    if (!connectionsEnabled())
        return;

    for (const auto &connection : map->getConnections()) {
        if (!m_settings.showConnections.contains(connection->direction()))
            continue;
        painter->drawImage(connection->relativePos(true) * 16, connection->render().toImage());
    }
}

void MapImageExporter::paintEvents(QPainter *painter, const Map *map) {
    if (!eventsEnabled())
        return;

    auto savedOpacity = painter->opacity();
    for (const auto &group : Event::groups()) {
        if (!m_settings.showEvents.contains(group))
            continue;
        for (const auto &event : map->getEvents(group)) {
            m_project->loadEventPixmap(event);
            if (m_mode != ImageExporterMode::Timelapse) {
                // GIF format doesn't support partial transparency, so we can't do this in Timelapse mode.
                painter->setOpacity(event->getUsesDefaultPixmap() ? 0.7 : 1.0);
            }
            painter->drawImage(QPoint(event->getPixelX(), event->getPixelY()), event->getPixmap().toImage());
        }
    }
    painter->setOpacity(savedOpacity);
}

void MapImageExporter::paintGrid(QPainter *painter, const Layout *layout) {
    if (!m_settings.showGrid)
        return;

    int w = layout->getWidth() * 16;
    int h = layout->getHeight() * 16;
    for (int x = 0; x <= w; x += 16) {
        painter->drawLine(x, 0, x, h);
    }
    for (int y = 0; y <= h; y += 16) {
        painter->drawLine(0, y, w, y);
    }
}

bool MapImageExporter::eventsEnabled() {
    return !m_settings.showEvents.isEmpty();
}

void MapImageExporter::setEventGroupEnabled(Event::Group group, bool enable) {
    if (enable) {
        m_settings.showEvents.insert(group);
    } else {
        m_settings.showEvents.remove(group);
    }
}

bool MapImageExporter::connectionsEnabled() {
    return !m_settings.showConnections.isEmpty();
}

void MapImageExporter::setConnectionDirectionEnabled(const QString &dir, bool enable) {
    if (enable) {
        m_settings.showConnections.insert(dir);
    } else {
        m_settings.showConnections.remove(dir);
    }
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
    setEventGroupEnabled(Event::Group::Object, state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Warps_stateChanged(int state) {
    setEventGroupEnabled(Event::Group::Warp, state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_BGs_stateChanged(int state) {
    setEventGroupEnabled(Event::Group::Bg, state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Triggers_stateChanged(int state) {
    setEventGroupEnabled(Event::Group::Coord, state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_HealLocations_stateChanged(int state) {
    setEventGroupEnabled(Event::Group::Heal, state == Qt::Checked);
    updatePreview();
}

// Shortcut setting for enabling all events
void MapImageExporter::on_checkBox_AllEvents_stateChanged(int state) {
    bool on = (state == Qt::Checked);

    const QSignalBlocker b_Objects(ui->checkBox_Objects);
    ui->checkBox_Objects->setChecked(on);
    ui->checkBox_Objects->setDisabled(on);
    setEventGroupEnabled(Event::Group::Object, on);

    const QSignalBlocker b_Warps(ui->checkBox_Warps);
    ui->checkBox_Warps->setChecked(on);
    ui->checkBox_Warps->setDisabled(on);
    setEventGroupEnabled(Event::Group::Warp, on);

    const QSignalBlocker b_BGs(ui->checkBox_BGs);
    ui->checkBox_BGs->setChecked(on);
    ui->checkBox_BGs->setDisabled(on);
    setEventGroupEnabled(Event::Group::Bg, on);

    const QSignalBlocker b_Triggers(ui->checkBox_Triggers);
    ui->checkBox_Triggers->setChecked(on);
    ui->checkBox_Triggers->setDisabled(on);
    setEventGroupEnabled(Event::Group::Coord, on);

    const QSignalBlocker b_HealLocations(ui->checkBox_HealLocations);
    ui->checkBox_HealLocations->setChecked(on);
    ui->checkBox_HealLocations->setDisabled(on);
    setEventGroupEnabled(Event::Group::Heal, on);

    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionUp_stateChanged(int state) {
    setConnectionDirectionEnabled("up", state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionDown_stateChanged(int state) {
    setConnectionDirectionEnabled("down", state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionLeft_stateChanged(int state) {
    setConnectionDirectionEnabled("left", state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionRight_stateChanged(int state) {
    setConnectionDirectionEnabled("right", state == Qt::Checked);
    updatePreview();
}

// Shortcut setting for enabling all connection directions
void MapImageExporter::on_checkBox_AllConnections_stateChanged(int state) {
    bool on = (state == Qt::Checked);

    const QSignalBlocker b_Up(ui->checkBox_ConnectionUp);
    ui->checkBox_ConnectionUp->setChecked(on);
    ui->checkBox_ConnectionUp->setDisabled(on);
    setConnectionDirectionEnabled("up", on);

    const QSignalBlocker b_Down(ui->checkBox_ConnectionDown);
    ui->checkBox_ConnectionDown->setChecked(on);
    ui->checkBox_ConnectionDown->setDisabled(on);
    setConnectionDirectionEnabled("down", on);

    const QSignalBlocker b_Left(ui->checkBox_ConnectionLeft);
    ui->checkBox_ConnectionLeft->setChecked(on);
    ui->checkBox_ConnectionLeft->setDisabled(on);
    setConnectionDirectionEnabled("left", on);

    const QSignalBlocker b_Right(ui->checkBox_ConnectionRight);
    ui->checkBox_ConnectionRight->setChecked(on);
    ui->checkBox_ConnectionRight->setDisabled(on);
    setConnectionDirectionEnabled("right", on);

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

    const QSignalBlocker b_TimelapseDelay(ui->spinBox_TimelapseDelay);
    ui->spinBox_TimelapseDelay->setValue(m_settings.timelapseDelayMs);

    const QSignalBlocker b_FrameSkip(ui->spinBox_FrameSkip);
    ui->spinBox_FrameSkip->setValue(m_settings.timelapseSkipAmount);

    updatePreview();
}

void MapImageExporter::on_spinBox_TimelapseDelay_valueChanged(int delayMs) {
    m_settings.timelapseDelayMs = delayMs;
    updatePreview();
}

void MapImageExporter::on_spinBox_FrameSkip_valueChanged(int skip) {
    m_settings.timelapseSkipAmount = skip;
    updatePreview();
}
