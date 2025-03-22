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
    m_mode(mode)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    m_scene = new QGraphicsScene(this);
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
    delete m_timelapseImage;
    delete ui;
}

void MapImageExporter::setModeSpecificUi() {
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
    } else if (m_project->layoutIds.contains(text) && m_mode != ImageExporterMode::Stitch) {
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
                if (m_preview->pixmap().isNull()) {
                    updatePreview();
                    if (m_preview->pixmap().isNull())
                        return; // Canceled
                }
                m_preview->pixmap().save(filepath);
                break;
            case ImageExporterMode::Timelapse:
                if (!m_timelapseImage || m_timelapseImage->frameCount() == 0) {
                    m_timelapseImage = createTimelapseImage();
                    if (!m_timelapseImage || m_timelapseImage->frameCount() == 0)
                        return; // Canceled
                }
                m_timelapseImage->save(filepath);
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
            return connectionsEnabled();
        case CommandId::ID_EventMove:
        case CommandId::ID_EventShift:
        case CommandId::ID_EventCreate:
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
    QRect dimensions(0,0, m_map->getWidth(), m_map->getHeight());
    for (const StitchedMap &map : stitchedMaps) {
        dimensions.setLeft(qMin(dimensions.left(), map.x));
        dimensions.setTop(qMin(dimensions.top(), map.y));
        dimensions.setRight(qMax(dimensions.right(), map.x + (map.map->getWidth()) * 16));
        dimensions.setBottom(qMax(dimensions.bottom(), map.y + (map.map->getHeight()) * 16));
    }

    // Adjust overall dimensions to account for elements at the edge (the border, grid, etc.)
    QMargins margins = getMargins();
    dimensions += margins;

    QPixmap stitchedPixmap(dimensions.width(), dimensions.height());
    stitchedPixmap.fill(Qt::black);

    QPainter painter(&stitchedPixmap);
    painter.translate(-dimensions.left(), -dimensions.top());

    // Borders can occlude neighboring maps, so we draw all the borders before drawing any maps.
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

    // Draw the layouts and collision images.
    // There's no chance these will overlap with neighbors, so we can do both in a single pass.
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
    if (eventsEnabled()) {
        progress->setLabelText("Drawing map events...");
        progress->setValue(0);
        numDrawn = 0;
        for (const StitchedMap &map : stitchedMaps) {
            if (progress->wasCanceled()) {
                return QPixmap();
            }
            painter.translate(map.x, map.y);
            paintEvents(&painter, map.map);
            painter.translate(-map.x, -map.y);

            progress->setValue(numDrawn++);
        }
    }

    // Nothing should be on top of the grid, so it's drawn last.
    if (m_settings.showGrid) {
        progress->setLabelText("Drawing map grids...");
        progress->setValue(0);
        numDrawn = 0;
        for (const StitchedMap &map : stitchedMaps) {
            if (progress->wasCanceled()) {
                return QPixmap();
            }
            painter.translate(map.x, map.y);
            paintGrid(&painter, map.map->layout());
            painter.translate(-map.x, -map.y);

            progress->setValue(numDrawn++);
        }
    }

    return stitchedPixmap;
}

QGifImage* MapImageExporter::createTimelapseImage() {
    // Timelapse will play in order of layout changes then map changes (events)
    // TODO: potentially update in the future?
    auto timelapseImg = new QGifImage();
    timelapseImg->setDefaultDelay(m_settings.timelapseDelayMs);
    timelapseImg->setDefaultTransparentColor(QColor(0, 0, 0));

    // lambda to avoid redundancy
    auto generateTimelapseFromHistory = [this, timelapseImg](QString progressText, QUndoStack *historyStack){
        QProgressDialog progress(progressText, "Cancel", 0, 1, this);
        progress.setAutoClose(true);
        progress.setWindowModality(Qt::WindowModal);
        progress.setModal(true);
        progress.setMaximum(1);
        progress.setValue(0);

        QMargins margins = getMargins(m_map); // m_map may be nullptr here, that's ok.
        int maxWidth = m_layout->getWidth() * 16 + margins.left() + margins.right();
        int maxHeight = m_layout->getHeight() * 16 + margins.top() + margins.bottom();

        // Rewind to the specified start of the map edit history.
        int i = 0;
        while (historyStack->canUndo()) {
            historyStack->undo();
            margins = getMargins(m_map);
            maxWidth = qMax(maxWidth, m_layout->getWidth() * 16 + margins.left() + margins.right());
            maxHeight = qMax(maxHeight, m_layout->getHeight() * 16 + margins.top() + margins.bottom());
            i++;
        }

        // Draw each frame, skipping the specified number of map edits in
        // the undo history.
        progress.setMaximum(i);
        while (i > 0) {
            if (progress.wasCanceled()) {
                progress.close();
                while (i > 0 && historyStack->canRedo()) {
                    i--;
                    historyStack->redo();
                }
                return false;
            }
            while (historyStack->canRedo() &&
                   !historyItemAppliesToFrame(historyStack->command(historyStack->index()))) {
                i--;
                historyStack->redo();
            }
            progress.setValue(progress.maximum() - i);
            QPixmap pixmap = getFormattedMapPixmap();
            if (pixmap.width() < maxWidth || pixmap.height() < maxHeight) {
                QPixmap resizedPixmap = QPixmap(maxWidth, maxHeight);
                QPainter painter(&resizedPixmap);
                resizedPixmap.fill(QColor(0, 0, 0));
                painter.drawPixmap(0, 0, pixmap.width(), pixmap.height(), pixmap);
                painter.end();
                pixmap = resizedPixmap;
            }
            timelapseImg->addFrame(pixmap.toImage());
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
        timelapseImg->addFrame(pixmap.toImage());
        progress.close();
        return true;
    };

    if (m_layout && !generateTimelapseFromHistory("Building layout timelapse...", &m_layout->editHistory)) {
        delete timelapseImg;
        return nullptr;
    }

    if (m_map && !generateTimelapseFromHistory("Building map timelapse...", m_map->editHistory())) {
        delete timelapseImg;
        return nullptr;
    }

    return timelapseImg;    
}

void MapImageExporter::updatePreview() {
    QPixmap previewPixmap;
    if (m_mode == ImageExporterMode::Stitch) { 
        QProgressDialog progress("Building map stitch...", "Cancel", 0, 1, this);
        progress.setAutoClose(true);
        progress.setWindowModality(Qt::WindowModal);
        progress.setModal(true);
        progress.setMinimumDuration(1000);
        previewPixmap = getStitchedImage(&progress);
        progress.close();
    } else if (m_mode == ImageExporterMode::Timelapse) {
        delete m_timelapseImage;
        m_timelapseImage = createTimelapseImage();

        // We want to convert the QGifImage data into a QMovie for the preview display.
        // Both support input/output with a QIODevice, so we use a QBuffer to translate the data.
        delete m_timelapseBuffer;
        m_timelapseBuffer = new QBuffer(this);
        m_timelapseBuffer->open(QBuffer::ReadWrite);
        m_timelapseImage->save(m_timelapseBuffer);
        m_timelapseBuffer->close();

        delete m_timelapseMovie;
        m_timelapseMovie = new QMovie(m_timelapseBuffer, "gif", this);
        m_timelapseMovie->setCacheMode(QMovie::CacheAll);
        connect(m_timelapseMovie, &QMovie::frameChanged, [this](int) {
            if (m_preview) m_preview->setPixmap(m_timelapseMovie->currentPixmap());
        });
        m_timelapseMovie->start();
        previewPixmap = m_timelapseMovie->currentPixmap();

    } else if (m_mode == ImageExporterMode::Normal) {
        previewPixmap = getFormattedMapPixmap();
    }

    if (m_preview) {
        if (m_preview->scene())
            m_preview->scene()->removeItem(m_preview);
        delete m_preview;
    }
    m_preview = m_scene->addPixmap(previewPixmap);
    m_scene->setSceneRect(m_scene->itemsBoundingRect());
    scalePreview();
}

void MapImageExporter::scalePreview() {
    if (!m_preview || m_settings.previewActualSize)
        return;
    ui->graphicsView_Preview->fitInView(m_preview, Qt::KeepAspectRatioByExpanding);
}

QPixmap MapImageExporter::getFormattedMapPixmap() {
    return m_map ? getFormattedMapPixmap(m_map) : getFormattedLayoutPixmap(m_layout);
}

QPixmap MapImageExporter::getFormattedLayoutPixmap(Layout *layout) {
    if (!layout)
        return QPixmap();

    QMargins margins = getMargins();
    QPixmap pixmap = getResizedPixmap(layout->render(true), margins);

    QPainter painter(&pixmap);
    painter.translate(margins.left(), margins.top());

    paintCollision(&painter, layout);
    paintBorder(&painter, layout);
    paintGrid(&painter, layout);

    return pixmap;
}

QPixmap MapImageExporter::getFormattedMapPixmap(Map *map) {
    if (!map)
        return QPixmap();

    QMargins margins = getMargins(map);
    QPixmap pixmap = getResizedPixmap(map->layout()->render(true), margins);

    QPainter painter(&pixmap);
    painter.translate(margins.left(), margins.top());

    paintCollision(&painter, map->layout());
    paintBorder(&painter, map->layout());
    paintConnections(&painter, map);
    paintEvents(&painter, map);
    paintGrid(&painter, map->layout());

    return pixmap;
}

QMargins MapImageExporter::getMargins(const Map *map) {
    QMargins margins;

    const int borderPixelWidth = BORDER_DISTANCE * 16;
    const int borderPixelHeight = BORDER_DISTANCE * 16;
    if (m_settings.showBorder) {
        margins = QMargins(borderPixelWidth, borderPixelHeight, borderPixelWidth, borderPixelHeight);
    } else if (map && connectionsEnabled()) {
        for (const auto &connection : map->getConnections()) {
            const QString dir = connection->direction();
            if (!m_settings.showConnections.contains(dir))
                continue;
            if (dir == "up") margins.setTop(borderPixelHeight);
            else if (dir == "down") margins.setBottom(borderPixelHeight);
            else if (dir == "left") margins.setLeft(borderPixelWidth);
            else if (dir == "right") margins.setRight(borderPixelWidth);
        }
    }

    if (m_settings.showGrid) {
        margins += QMargins(0, 0, 1, 1);
    }
    return margins;
}

QPixmap MapImageExporter::getResizedPixmap(const QPixmap &pixmap, const QMargins &margins) {
    if (margins.isNull())
        return pixmap;

    QPixmap resizedPixmap = QPixmap(pixmap.width() + margins.left() + margins.right(),
                                    pixmap.height() + margins.top() + margins.bottom());
    resizedPixmap.fill(Qt::black);

    QPainter painter(&resizedPixmap);
    painter.drawPixmap(margins.left(), margins.top(), pixmap);
    painter.end();
    return resizedPixmap;
}

void MapImageExporter::paintCollision(QPainter *painter, Layout *layout) {
    if (!m_settings.showCollision)
        return;

    layout->renderCollision(true);

    auto savedOpacity = painter->opacity();
    painter->setOpacity(static_cast<qreal>(porymapConfig.collisionOpacity) / 100);
    painter->drawPixmap(0, 0, layout->collision_pixmap);
    painter->setOpacity(savedOpacity);
}

// TODO: Route109 map border has an empty row?
void MapImageExporter::paintBorder(QPainter *painter, Layout *layout) {
    if (!m_settings.showBorder)
        return;

    layout->renderBorder();

    int borderHorzDist = layout->getBorderDrawWidth();
    int borderVertDist = layout->getBorderDrawHeight();
    for (int y = -borderVertDist; y < layout->getHeight() + borderVertDist; y += layout->getBorderHeight())
    for (int x = -borderHorzDist; x < layout->getWidth() + borderHorzDist; x += layout->getBorderWidth()) {
        if (layout->isWithinBounds(x, y)) continue; // Skip border painting if it would be covered by the rest of the map.
        painter->drawPixmap(x * 16, y * 16, layout->border_pixmap);
    }
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
