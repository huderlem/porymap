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

    m_scene = new CheckeredBgScene(QSize(8,8), this);
    m_preview = m_scene->addPixmap(QPixmap());
    ui->graphicsView_Preview->setScene(m_scene);

    setModeSpecificUi();

    connect(ui->pushButton_Save,   &QPushButton::pressed, this, &MapImageExporter::saveImage);
    connect(ui->pushButton_Cancel, &QPushButton::pressed, this, &MapImageExporter::close);

    connect(ui->comboBox_MapSelection, &NoScrollComboBox::editingFinished, this, &MapImageExporter::updateMapSelection);

    connect(ui->checkBox_Objects,               &QCheckBox::toggled, this, &MapImageExporter::setShowObjects);
    connect(ui->checkBox_Warps,                 &QCheckBox::toggled, this, &MapImageExporter::setShowWarps);
    connect(ui->checkBox_BGs,                   &QCheckBox::toggled, this, &MapImageExporter::setShowBgs);
    connect(ui->checkBox_Triggers,              &QCheckBox::toggled, this, &MapImageExporter::setShowTriggers);
    connect(ui->checkBox_HealLocations,         &QCheckBox::toggled, this, &MapImageExporter::setShowHealLocations);
    connect(ui->checkBox_AllEvents,             &QCheckBox::toggled, this, &MapImageExporter::setShowAllEvents);
    connect(ui->checkBox_ConnectionUp,          &QCheckBox::toggled, this, &MapImageExporter::setShowConnectionUp);
    connect(ui->checkBox_ConnectionDown,        &QCheckBox::toggled, this, &MapImageExporter::setShowConnectionDown);
    connect(ui->checkBox_ConnectionLeft,        &QCheckBox::toggled, this, &MapImageExporter::setShowConnectionLeft);
    connect(ui->checkBox_ConnectionRight,       &QCheckBox::toggled, this, &MapImageExporter::setShowConnectionRight);
    connect(ui->checkBox_AllConnections,        &QCheckBox::toggled, this, &MapImageExporter::setShowAllConnections);
    connect(ui->checkBox_Collision,             &QCheckBox::toggled, this, &MapImageExporter::setShowCollision);
    connect(ui->checkBox_Grid,                  &QCheckBox::toggled, this, &MapImageExporter::setShowGrid);
    connect(ui->checkBox_Border,                &QCheckBox::toggled, this, &MapImageExporter::setShowBorder);
    connect(ui->checkBox_DisablePreviewScaling, &QCheckBox::toggled, this, &MapImageExporter::setDisablePreviewScaling);
    connect(ui->checkBox_DisablePreviewUpdates, &QCheckBox::toggled, this, &MapImageExporter::setDisablePreviewUpdates);

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
        ui->comboBox_MapSelection->addItems(m_project->mapNames());
        ui->comboBox_MapSelection->setTextItem(m_map->name());
        ui->label_MapSelection->setText(m_mode == ImageExporterMode::Stitch ? QStringLiteral("Starting Map") : QStringLiteral("Map"));
    } else if (m_layout) {
        ui->comboBox_MapSelection->addItems(m_project->layoutIds());
        ui->comboBox_MapSelection->setTextItem(m_layout->id);
        ui->label_MapSelection->setText(QStringLiteral("Layout"));
    }

    if (m_mode == ImageExporterMode::Timelapse) {
        // TODO: At the moment edit history for events explicitly depend on the editor and assume their map is currently open.
        // Other edit commands rely on this more subtly, like triggering API callbacks or
        // spending time rendering their layout (which can make creating timelapses very slow).
        // Until this is resolved, the selected map/layout must remain the same as in the editor.
        // We enforce this here by disabling the selector, and in MainWindow by programmatically
        // changing the exporter's map/layout selection if the user opens a new one in the editor.
        ui->comboBox_MapSelection->setEnabled(false);
        ui->label_MapSelection->setEnabled(false);
    }

    // Update for any mode-specific default settings
    resetSettings();
}

// Allow the window to open before displaying the preview.
void MapImageExporter::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (!event->spontaneous())
        QTimer::singleShot(0, this, [this](){ updatePreview(); });
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
    ui->comboBox_MapSelection->setTextItem(text);
    updateMapSelection();
}

void MapImageExporter::updateMapSelection() {
    auto oldMap = m_map;
    auto oldLayout = m_layout;

    const QString text = ui->comboBox_MapSelection->currentText();
    if (m_project->isKnownMap(text)) {
        auto newMap = m_project->loadMap(text);
        if (newMap) {
            m_map = newMap;
            m_layout = newMap->layout();
        }
    } else if (m_project->isKnownLayout(text)) {
        auto newLayout = m_project->loadLayout(text);
        if (newLayout) {
            m_map = nullptr;
            m_layout = newLayout;
        }
    }

    // Ensure text in the combo box remains valid
    const QSignalBlocker b(ui->comboBox_MapSelection);
    ui->comboBox_MapSelection->setTextItem(m_map ? m_map->name() : m_layout->id);

    if (m_map != oldMap && (!m_map || !oldMap)) {
        // Switching to or from layout-only mode
        setModeSpecificUi();
    }
    if (m_map != oldMap || m_layout != oldLayout){
        updatePreview();
    }
}

void MapImageExporter::saveImage() {
    // If the preview is empty (because progress was canceled) or if updates were disabled
    // then we should ensure the image in the preview is up-to-date before exporting.
    if (m_previewImage.isNull() || m_settings.disablePreviewUpdates) {
        updatePreview(true);
        if (m_previewImage.isNull())
            return; // Canceled
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
                m_previewImage.save(filepath);
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
        case CommandId::ID_MapConnectionRemove: {
            if (!connectionsEnabled())
                return false;
            uint32_t flags = 0;
            if (m_settings.showConnections.contains("up"))    flags |= IDMask_ConnectionDirection_Up;
            if (m_settings.showConnections.contains("down"))  flags |= IDMask_ConnectionDirection_Down;
            if (m_settings.showConnections.contains("left"))  flags |= IDMask_ConnectionDirection_Left;
            if (m_settings.showConnections.contains("right")) flags |= IDMask_ConnectionDirection_Right;
            return (command->id() & flags) != 0;
        }
        case CommandId::ID_EventMove:
        case CommandId::ID_EventShift:
        case CommandId::ID_EventCreate:
        case CommandId::ID_EventPaste:
        case CommandId::ID_EventDelete:
        case CommandId::ID_EventDuplicate: {
            if (!eventsEnabled())
                return false;
            uint32_t flags = 0;
            if (m_settings.showEvents.contains(Event::Group::Object)) flags |= IDMask_EventType_Object;
            if (m_settings.showEvents.contains(Event::Group::Warp))   flags |= IDMask_EventType_Warp;
            if (m_settings.showEvents.contains(Event::Group::Bg))     flags |= IDMask_EventType_BG;
            if (m_settings.showEvents.contains(Event::Group::Coord))  flags |= IDMask_EventType_Trigger;
            if (m_settings.showEvents.contains(Event::Group::Heal))   flags |= IDMask_EventType_Heal;
            return (command->id() & flags) != 0;
        }
        default:
            return false;
    }
}

QImage MapImageExporter::getExpandedImage(const QImage &image, const QSize &targetSize, const QColor &fillColor) {
    if (image.width() >= targetSize.width() && image.height() >= targetSize.height())
        return image;

    QImage resizedImage(targetSize, QImage::Format_RGBA8888);
    QPainter painter(&resizedImage);
    resizedImage.fill(fillColor);

    // Center the old image in the new resized one.
    int x = (targetSize.width() - image.width()) / 2;
    int y = (targetSize.height() - image.height()) / 2;
    painter.drawImage(x, y, image);
    return resizedImage;
}

struct TimelapseStep {
    QUndoStack* historyStack;
    int initialStackIndex;
    QString name;
};

QGifImage* MapImageExporter::createTimelapseGifImage(QProgressDialog *progress) {
    // TODO: Timelapse will play in order of layout changes then map changes (events, connections). Potentially update in the future?
    QList<TimelapseStep> steps;
    steps.append({
        .historyStack = &m_layout->editHistory,
        .initialStackIndex = m_layout->editHistory.index(),
        .name = "layout",
    });
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
            if (currentHistoryAppliesToFrame(step.historyStack) || step.historyStack->index() == step.initialStackIndex) {
                // Either this is relevant edit history, or it's the final frame (which is always rendered). Record the size of the map at this point.
                QMargins margins = getMargins(m_map);
                canvasSize = canvasSize.expandedTo(QSize(m_layout->pixelWidth() + margins.left() + margins.right(),
                                                         m_layout->pixelHeight() + margins.top() + margins.bottom()));
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
                timelapseImg->addFrame(getExpandedImage(getFormattedMapImage(), canvasSize, m_settings.fillColor));
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
        timelapseImg->addFrame(getExpandedImage(getFormattedMapImage(), canvasSize, m_settings.fillColor));
    }
    return timelapseImg;
}

struct StitchedMap {
    int x;
    int y;
    Map* map;
};

QImage MapImageExporter::getStitchedImage(QProgressDialog *progress) {
    // Do a breadth-first search to gather a collection of
    // all reachable maps with their relative offsets.
    QSet<QString> visited;
    QList<StitchedMap> stitchedMaps;
    QList<StitchedMap> unvisited;
    unvisited.append(StitchedMap{0, 0, m_map});

    progress->setLabelText("Gathering stitched maps...");
    while (!unvisited.isEmpty()) {
        if (progress->wasCanceled()) {
            return QImage();
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
            QPoint pos = connection->relativePixelPos();
            unvisited.append(StitchedMap{cur.x + pos.x(), cur.y + pos.y(), connectedMap});
        }
    }
    if (stitchedMaps.isEmpty())
        return QImage();

    progress->setMaximum(stitchedMaps.size());
    int numDrawn = 0;

    // Determine the overall dimensions of the stitched maps.
    QRect dimensions = QRect(0, 0, m_map->getWidth(), m_map->getHeight()) + getMargins(m_map);
    for (const StitchedMap &map : stitchedMaps) {
        dimensions |= (QRect(map.x, map.y, map.map->pixelWidth(), map.map->pixelHeight()) + getMargins(map.map));
    }

    QImage stitchedImage(dimensions.width(), dimensions.height(), QImage::Format_RGBA8888);
    stitchedImage.fill(m_settings.fillColor);

    QPainter painter(&stitchedImage);
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
                return QImage();
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
            return QImage();
        }
        map.map->layout()->render(true);
        painter.translate(map.x, map.y);
        painter.drawImage(0, 0, map.map->layout()->image);
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
                return QImage();
            }
            painter.translate(map.x, map.y);
            paintEvents(&painter, map.map);
            paintGrid(&painter, map.map->layout());
            painter.translate(-map.x, -map.y);

            progress->setValue(numDrawn++);
        }
    }

    return stitchedImage;
}

void MapImageExporter::updatePreview(bool forceUpdate) {
    if (m_settings.disablePreviewUpdates && !forceUpdate)
        return;

    QProgressDialog progress("", "Cancel", 0, 1, this);
    progress.setAutoClose(true);
    progress.setWindowModality(Qt::WindowModal);
    progress.setModal(true);
    progress.setMinimumDuration(1000);

    if (m_mode == ImageExporterMode::Normal) {
        m_previewImage = getFormattedMapImage();
    } else if (m_mode == ImageExporterMode::Stitch) {
        m_previewImage = getStitchedImage(&progress);
    } else if (m_mode == ImageExporterMode::Timelapse) {
        if (m_timelapseMovie)
            m_timelapseMovie->stop();

        m_timelapseGifImage = createTimelapseGifImage(&progress);
        if (!m_timelapseGifImage) {
            m_previewImage = QImage();
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
            m_previewImage = m_timelapseMovie->currentImage();
        }
    } else {
        m_previewImage = QImage();
    }
    progress.close();

    m_previewImage.setColorSpace(Util::toColorSpace(porymapConfig.imageExportColorSpaceId));
    m_preview->setPixmap(QPixmap::fromImage(m_previewImage));
    m_scene->setSceneRect(m_scene->itemsBoundingRect());
    scalePreview();
}

void MapImageExporter::scalePreview() {
    if (!m_preview || m_settings.disablePreviewScaling)
        return;
    ui->graphicsView_Preview->fitInView(m_preview, Qt::KeepAspectRatioByExpanding);
}

QImage MapImageExporter::getFormattedMapImage() {
    if (!m_layout)
        return QImage();

    m_layout->render(true);

    // Create image large enough to contain the map and the marginal elements (the border, grid, etc.)
    QMargins margins = getMargins(m_map);
    QImage image(m_layout->image.width() + margins.left() + margins.right(),
                 m_layout->image.height() + margins.top() + margins.bottom(),
                 QImage::Format_RGBA8888);
    image.fill(m_settings.fillColor);

    QPainter painter(&image);
    painter.translate(margins.left(), margins.top());

    paintBorder(&painter, m_layout);
    painter.drawImage(0, 0, m_layout->image);
    paintCollision(&painter, m_layout);
    if (m_map) {
        paintConnections(&painter, m_map);
        paintEvents(&painter, m_map);
    }
    paintGrid(&painter, m_layout);

    return image;
}

QMargins MapImageExporter::getMargins(const Map *map) {
    QMargins margins;
    if (m_settings.showBorder) {
        margins = m_project->getPixelViewDistance();
    } else if (map && connectionsEnabled()) {
        for (const auto &connection : map->getConnections()) {
            const QString dir = connection->direction();
            if (!m_settings.showConnections.contains(dir))
                continue;
            auto targetMap = connection->targetMap();
            if (!targetMap) continue;

            QRect rect = targetMap->getConnectionRect(dir);
            if (dir == "up") margins.setTop(rect.height());
            else if (dir == "down") margins.setBottom(rect.height());
            else if (dir == "left") margins.setLeft(rect.width());
            else if (dir == "right") margins.setRight(rect.width());
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

    layout->renderCollision(true);

    auto savedOpacity = painter->opacity();
    painter->setOpacity(static_cast<qreal>(porymapConfig.collisionOpacity) / 100);
    painter->drawImage(0, 0, layout->collision_image);
    painter->setOpacity(savedOpacity);
}

void MapImageExporter::paintBorder(QPainter *painter, Layout *layout) {
    if (!m_settings.showBorder)
        return;

    layout->renderBorder(true);

    // Clip parts of the border that would be beyond player visibility.
    painter->save();
    painter->setClipRect(layout->getVisibleRect());

    const QMargins borderMargins = layout->getBorderMargins();
    for (int y = -borderMargins.top(); y < layout->getHeight() + borderMargins.bottom(); y += layout->getBorderHeight())
    for (int x = -borderMargins.left(); x < layout->getWidth() + borderMargins.right(); x += layout->getBorderWidth()) {
         // Skip border painting if it would be fully covered by the rest of the map
        if (layout->isWithinBounds(QRect(x, y, layout->getBorderWidth(), layout->getBorderHeight())))
            continue;
        painter->drawImage(x * Metatile::pixelWidth(), y * Metatile::pixelHeight(), layout->border_image);
    }

    painter->restore();
}

void MapImageExporter::paintConnections(QPainter *painter, const Map *map) {
    if (!connectionsEnabled())
        return;

    for (const auto &connection : map->getConnections()) {
        if (!m_settings.showConnections.contains(connection->direction()))
            continue;
        painter->drawImage(connection->relativePixelPos(true), connection->renderImage());
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

    int w = layout->pixelWidth();
    int h = layout->pixelHeight();
    for (int x = 0; x <= w; x += Metatile::pixelWidth()) {
        painter->drawLine(x, 0, x, h);
    }
    for (int y = 0; y <= h; y += Metatile::pixelHeight()) {
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
    return !m_settings.showConnections.isEmpty() && m_mode != ImageExporterMode::Stitch;
}

void MapImageExporter::setConnectionDirectionEnabled(const QString &dir, bool enable) {
    if (enable) {
        m_settings.showConnections.insert(dir);
    } else {
        m_settings.showConnections.remove(dir);
    }
}

void MapImageExporter::setShowCollision(bool checked) {
    m_settings.showCollision = checked;
    updatePreview();
}

void MapImageExporter::setShowGrid(bool checked) {
    m_settings.showGrid = checked;
    updatePreview();
}

void MapImageExporter::setShowBorder(bool checked) {
    m_settings.showBorder = checked;
    updatePreview();
}

void MapImageExporter::setShowObjects(bool checked) {
    setEventGroupEnabled(Event::Group::Object, checked);
    updatePreview();
}

void MapImageExporter::setShowWarps(bool checked) {
    setEventGroupEnabled(Event::Group::Warp, checked);
    updatePreview();
}

void MapImageExporter::setShowBgs(bool checked) {
    setEventGroupEnabled(Event::Group::Bg, checked);
    updatePreview();
}

void MapImageExporter::setShowTriggers(bool checked) {
    setEventGroupEnabled(Event::Group::Coord, checked);
    updatePreview();
}

void MapImageExporter::setShowHealLocations(bool checked) {
    setEventGroupEnabled(Event::Group::Heal, checked);
    updatePreview();
}

// Shortcut setting for enabling all events
void MapImageExporter::setShowAllEvents(bool checked) {
    const QSignalBlocker b_Objects(ui->checkBox_Objects);
    ui->checkBox_Objects->setChecked(checked);
    ui->checkBox_Objects->setDisabled(checked);
    setEventGroupEnabled(Event::Group::Object, checked);

    const QSignalBlocker b_Warps(ui->checkBox_Warps);
    ui->checkBox_Warps->setChecked(checked);
    ui->checkBox_Warps->setDisabled(checked);
    setEventGroupEnabled(Event::Group::Warp, checked);

    const QSignalBlocker b_BGs(ui->checkBox_BGs);
    ui->checkBox_BGs->setChecked(checked);
    ui->checkBox_BGs->setDisabled(checked);
    setEventGroupEnabled(Event::Group::Bg, checked);

    const QSignalBlocker b_Triggers(ui->checkBox_Triggers);
    ui->checkBox_Triggers->setChecked(checked);
    ui->checkBox_Triggers->setDisabled(checked);
    setEventGroupEnabled(Event::Group::Coord, checked);

    const QSignalBlocker b_HealLocations(ui->checkBox_HealLocations);
    ui->checkBox_HealLocations->setChecked(checked);
    ui->checkBox_HealLocations->setDisabled(checked);
    setEventGroupEnabled(Event::Group::Heal, checked);

    updatePreview();
}

void MapImageExporter::setShowConnectionUp(bool checked) {
    setConnectionDirectionEnabled("up", checked);
    updatePreview();
}

void MapImageExporter::setShowConnectionDown(bool checked) {
    setConnectionDirectionEnabled("down", checked);
    updatePreview();
}

void MapImageExporter::setShowConnectionLeft(bool checked) {
    setConnectionDirectionEnabled("left", checked);
    updatePreview();
}

void MapImageExporter::setShowConnectionRight(bool checked) {
    setConnectionDirectionEnabled("right", checked);
    updatePreview();
}

// Shortcut setting for enabling all connection directions
void MapImageExporter::setShowAllConnections(bool checked) {
    const QSignalBlocker b_Up(ui->checkBox_ConnectionUp);
    ui->checkBox_ConnectionUp->setChecked(checked);
    ui->checkBox_ConnectionUp->setDisabled(checked);
    setConnectionDirectionEnabled("up", checked);

    const QSignalBlocker b_Down(ui->checkBox_ConnectionDown);
    ui->checkBox_ConnectionDown->setChecked(checked);
    ui->checkBox_ConnectionDown->setDisabled(checked);
    setConnectionDirectionEnabled("down", checked);

    const QSignalBlocker b_Left(ui->checkBox_ConnectionLeft);
    ui->checkBox_ConnectionLeft->setChecked(checked);
    ui->checkBox_ConnectionLeft->setDisabled(checked);
    setConnectionDirectionEnabled("left", checked);

    const QSignalBlocker b_Right(ui->checkBox_ConnectionRight);
    ui->checkBox_ConnectionRight->setChecked(checked);
    ui->checkBox_ConnectionRight->setDisabled(checked);
    setConnectionDirectionEnabled("right", checked);

    updatePreview();
}

void MapImageExporter::setDisablePreviewScaling(bool checked) {
    m_settings.disablePreviewScaling = checked;
    if (m_settings.disablePreviewScaling) {
        ui->graphicsView_Preview->resetTransform();
    } else {
        scalePreview();
    }
}

void MapImageExporter::setDisablePreviewUpdates(bool checked) {
    m_settings.disablePreviewUpdates = checked;
    if (m_settings.disablePreviewUpdates) {
        if (m_timelapseMovie) {
            m_timelapseMovie->stop();
        }
    } else {
        updatePreview();
    }
}

void MapImageExporter::on_pushButton_Reset_pressed() {
    resetSettings();
    updatePreview();
}

void MapImageExporter::resetSettings() {
     m_settings = {};

    for (auto widget : this->findChildren<QCheckBox *>()) {
        const QSignalBlocker b(widget); // Prevent calls to updatePreview
        widget->setChecked(false); // This assumes the default state of all checkboxes settings is false.
    }

    const QSignalBlocker b_TimelapseDelay(ui->spinBox_TimelapseDelay);
    ui->spinBox_TimelapseDelay->setValue(m_settings.timelapseDelayMs);

    const QSignalBlocker b_FrameSkip(ui->spinBox_FrameSkip);
    ui->spinBox_FrameSkip->setValue(m_settings.timelapseSkipAmount);

    if (m_mode == ImageExporterMode::Timelapse) {
        // Timelapse gif has artifacts with transparency, make sure it's disabled.
        m_settings.fillColor.setAlpha(255);
    }
}

// These spin boxes can be changed rapidly, so we wait for editing to finish before updating the preview.
void MapImageExporter::on_spinBox_TimelapseDelay_editingFinished() {
    int delayMs = ui->spinBox_TimelapseDelay->value();
    if (delayMs == m_settings.timelapseDelayMs)
        return;

    m_settings.timelapseDelayMs = delayMs;
    updatePreview();
}

void MapImageExporter::on_spinBox_FrameSkip_editingFinished() {
    int skipAmount = ui->spinBox_FrameSkip->value();
    if (skipAmount == m_settings.timelapseSkipAmount)
        return;

    m_settings.timelapseSkipAmount = skipAmount;
    updatePreview();
}
