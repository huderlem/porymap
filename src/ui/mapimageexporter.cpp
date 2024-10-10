#include "mapimageexporter.h"
#include "ui_mapimageexporter.h"
#include "qgifimage.h"
#include "editcommands.h"

#include <QFileDialog>
#include <QImage>
#include <QPainter>
#include <QPoint>

#define STITCH_MODE_BORDER_DISTANCE 2

QString getTitle(ImageExporterMode mode) {
    switch (mode)
    {
        case ImageExporterMode::Normal:
            return "Export Map Image";
        case ImageExporterMode::Stitch:
            return "Export Map Stitch Image";
        case ImageExporterMode::Timelapse:
            return "Export Map Timelapse Image";
    }
    return "";
}

QString getDescription(ImageExporterMode mode) {
    switch (mode)
    {
        case ImageExporterMode::Normal:
            return "Exports an image of the selected map.";
        case ImageExporterMode::Stitch:
            return "Exports a combined image of all the maps connected to the selected map.";
        case ImageExporterMode::Timelapse:
            return "Exports a GIF of the edit history for the selected map.";
    }
    return "";
}

MapImageExporter::MapImageExporter(QWidget *parent_, Editor *editor_, ImageExporterMode mode) :
    QDialog(parent_),
    ui(new Ui::MapImageExporter)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);
    this->map = editor_->map;
    this->editor = editor_;
    this->mode = mode;
    this->setWindowTitle(getTitle(this->mode));
    this->ui->label_Description->setText(getDescription(this->mode));
    this->ui->groupBox_Connections->setVisible(this->mode != ImageExporterMode::Stitch);
    this->ui->groupBox_Timelapse->setVisible(this->mode == ImageExporterMode::Timelapse);

    this->ui->comboBox_MapSelection->addItems(editor->project->mapNames);
    this->ui->comboBox_MapSelection->setCurrentText(map->name);
    this->ui->comboBox_MapSelection->setEnabled(false);// TODO: allow selecting map from drop-down

    connect(ui->pushButton_Save,   &QPushButton::pressed, this, &MapImageExporter::saveImage);
    connect(ui->pushButton_Cancel, &QPushButton::pressed, this, &MapImageExporter::close);
}

MapImageExporter::~MapImageExporter() {
    delete scene;
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

void MapImageExporter::saveImage() {
    // Make sure preview is up-to-date before we save.
    if (this->preview.isNull())
        updatePreview();
    if (this->preview.isNull())
        return;

    QString title = getTitle(this->mode);
    QString defaultFilename;
    switch (this->mode)
    {
        case ImageExporterMode::Normal:
            defaultFilename = map->name;
            break;
        case ImageExporterMode::Stitch:
            defaultFilename = QString("Stitch_From_%1").arg(map->name);
            break;
        case ImageExporterMode::Timelapse:
            defaultFilename = QString("Timelapse_%1").arg(map->name);
            break;
    }

    QString defaultFilepath = QString("%1/%2.%3")
            .arg(editor->project->importExportPath)
            .arg(defaultFilename)
            .arg(this->mode == ImageExporterMode::Timelapse ? "gif" : "png");
    QString filter = this->mode == ImageExporterMode::Timelapse ? "Image Files (*.gif)" : "Image Files (*.png *.jpg *.bmp)";
    QString filepath = QFileDialog::getSaveFileName(this, title, defaultFilepath, filter);
    if (!filepath.isEmpty()) {
        editor->project->setImportExportPath(filepath);
        switch (this->mode) {
            case ImageExporterMode::Normal:
            case ImageExporterMode::Stitch:
                // Normal and Stitch modes already have the image ready to go in the preview.
                this->preview.save(filepath);
                break;
            case ImageExporterMode::Timelapse:
                QProgressDialog progress("Building map timelapse...", "Cancel", 0, 1, this);
                progress.setAutoClose(true);
                progress.setWindowModality(Qt::WindowModal);
                progress.setModal(true);
                progress.setMaximum(1);
                progress.setValue(0);

                int maxWidth = this->map->getWidth() * 16;
                int maxHeight = this->map->getHeight() * 16;
                if (this->settings.showBorder) {
                    maxWidth += 2 * STITCH_MODE_BORDER_DISTANCE * 16;
                    maxHeight += 2 * STITCH_MODE_BORDER_DISTANCE * 16;
                }
                // Rewind to the specified start of the map edit history.
                int i = 0;
                while (this->map->editHistory.canUndo()) {
                    progress.setValue(i);
                    this->map->editHistory.undo();
                    int width = this->map->getWidth() * 16;
                    int height = this->map->getHeight() * 16;
                    if (this->settings.showBorder) {
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
                QGifImage timelapseImg(QSize(maxWidth, maxHeight));
                timelapseImg.setDefaultDelay(this->settings.timelapseDelayMs);
                timelapseImg.setDefaultTransparentColor(QColor(0, 0, 0));
                // Draw each frame, skpping the specified number of map edits in
                // the undo history.
                progress.setMaximum(i);
                while (i > 0) {
                    if (progress.wasCanceled()) {
                        progress.close();
                        while (i > 0 && this->map->editHistory.canRedo()) {
                            i--;
                            this->map->editHistory.redo();
                        }
                        return;
                    }
                    while (this->map->editHistory.canRedo() &&
                           !historyItemAppliesToFrame(this->map->editHistory.command(this->map->editHistory.index()))) {
                        i--;
                        this->map->editHistory.redo();
                    }
                    progress.setValue(progress.maximum() - i);
                    QPixmap pixmap = this->getFormattedMapPixmap(this->map);
                    if (pixmap.width() < maxWidth || pixmap.height() < maxHeight) {
                        QPixmap pixmap2 = QPixmap(maxWidth, maxHeight);
                        QPainter painter(&pixmap2);
                        pixmap2.fill(QColor(0, 0, 0));
                        painter.drawPixmap(0, 0, pixmap.width(), pixmap.height(), pixmap);
                        painter.end();
                        pixmap = pixmap2;
                    }
                    timelapseImg.addFrame(pixmap.toImage());
                    for (int j = 0; j < this->settings.timelapseSkipAmount; j++) {
                        if (i > 0) {
                            i--;
                            this->map->editHistory.redo();
                            while (this->map->editHistory.canRedo() &&
                                   !historyItemAppliesToFrame(this->map->editHistory.command(this->map->editHistory.index()))) {
                                i--;
                                this->map->editHistory.redo();
                            }
                        }
                    }
                }
                // The latest map state is the last animated frame.
                QPixmap pixmap = this->getFormattedMapPixmap(this->map);
                timelapseImg.addFrame(pixmap.toImage());
                timelapseImg.save(filepath);
                progress.close();
                break;
        }
        this->close();
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
        case CommandId::ID_ResizeMap:
        case CommandId::ID_ScriptEditMap:
            return true;
        case CommandId::ID_PaintCollision:
        case CommandId::ID_BucketFillCollision:
        case CommandId::ID_MagicFillCollision:
            return this->settings.showCollision;
        case CommandId::ID_PaintBorder:
            return this->settings.showBorder;
        case CommandId::ID_MapConnectionMove:
        case CommandId::ID_MapConnectionChangeDirection:
        case CommandId::ID_MapConnectionChangeMap:
        case CommandId::ID_MapConnectionAdd:
        case CommandId::ID_MapConnectionRemove:
            return this->settings.showUpConnections || this->settings.showDownConnections || this->settings.showLeftConnections || this->settings.showRightConnections;
        case CommandId::ID_EventMove:
        case CommandId::ID_EventShift:
        case CommandId::ID_EventCreate:
        case CommandId::ID_EventDelete:
        case CommandId::ID_EventDuplicate: {
            bool eventTypeIsApplicable =
                       (this->settings.showObjects       && (command->id() & IDMask_EventType_Object)  != 0)
                    || (this->settings.showWarps         && (command->id() & IDMask_EventType_Warp)    != 0)
                    || (this->settings.showBGs           && (command->id() & IDMask_EventType_BG)      != 0)
                    || (this->settings.showTriggers      && (command->id() & IDMask_EventType_Trigger) != 0)
                    || (this->settings.showHealLocations && (command->id() & IDMask_EventType_Heal)    != 0);
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
    unvisited.append(StitchedMap{0, 0, this->editor->map});

    progress->setLabelText("Gathering stitched maps...");
    while (!unvisited.isEmpty()) {
        if (progress->wasCanceled()) {
            return QPixmap();
        }
        progress->setMaximum(visited.size() + unvisited.size());
        progress->setValue(visited.size());

        StitchedMap cur = unvisited.takeFirst();
        if (visited.contains(cur.map->name))
            continue;
        visited.insert(cur.map->name);
        stitchedMaps.append(cur);

        for (MapConnection *connection : cur.map->getConnections()) {
            const QString direction = connection->direction();
            int x = cur.x;
            int y = cur.y;
            int offset = connection->offset();
            Map *connectionMap = connection->targetMap();
            if (!connectionMap)
                continue;
            if (direction == "up") {
                x += offset;
                y -= connectionMap->getHeight();
            } else if (direction == "down") {
                x += offset;
                y += cur.map->getHeight();
            } else if (direction == "left") {
                x -= connectionMap->getWidth();
                y += offset;
            } else if (direction == "right") {
                x += cur.map->getWidth();
                y += offset;
            } else {
                // Ignore Dive/Emerge connections and unrecognized directions
                continue;
            }
            unvisited.append(StitchedMap{x, y, connectionMap});
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
        QPixmap pixmap = this->getFormattedMapPixmap(map.map);
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
            QPixmap pixmapWithoutBorders = this->getFormattedMapPixmap(map.map, true);
            painter.drawPixmap(pixelX, pixelY, pixmapWithoutBorders);
        }
    }

    return stitchedPixmap;
}

void MapImageExporter::updatePreview() {
    if (this->scene) {
        delete this->scene;
        this->scene = nullptr;
    }
    this->scene = new QGraphicsScene;

    if (this->mode == ImageExporterMode::Stitch) { 
        QProgressDialog progress("Building map stitch...", "Cancel", 0, 1, this);
        progress.setAutoClose(true);
        progress.setWindowModality(Qt::WindowModal);
        progress.setModal(true);
        progress.setMinimumDuration(1000);
        this->preview = getStitchedImage(&progress, this->settings.showBorder);
        progress.close();
    } else {
        // Timelapse mode doesn't currently have a real preview. It just displays the current map as in Normal mode.
        this->preview = getFormattedMapPixmap(this->map);
    }
    this->scene->addPixmap(this->preview);
    ui->graphicsView_Preview->setScene(scene);
    scalePreview();
}

void MapImageExporter::scalePreview() {
    if (this->scene && !this->settings.previewActualSize){
       ui->graphicsView_Preview->fitInView(this->scene->sceneRect(), Qt::KeepAspectRatioByExpanding);
    }
}

QPixmap MapImageExporter::getFormattedMapPixmap(Map *map, bool ignoreBorder) {
    QPixmap pixmap;

    // draw background layer / base image
    map->render(true);
    pixmap = map->pixmap;

    if (this->settings.showCollision) {
        QPainter collisionPainter(&pixmap);
        map->renderCollision(true);
        collisionPainter.setOpacity(editor->collisionOpacity);
        collisionPainter.drawPixmap(0, 0, map->collision_pixmap);
        collisionPainter.end();
    }

    // draw map border
    // note: this will break when allowing map to be selected from drop down maybe
    int borderHeight = 0, borderWidth = 0;
    if (!ignoreBorder && this->settings.showBorder) {
        int borderDistance = this->mode ? STITCH_MODE_BORDER_DISTANCE : BORDER_DISTANCE;
        map->renderBorder();
        int borderHorzDist = editor->getBorderDrawDistance(map->getBorderWidth());
        int borderVertDist = editor->getBorderDrawDistance(map->getBorderHeight());
        borderWidth = borderDistance * 16;
        borderHeight = borderDistance * 16;
        QPixmap newPixmap = QPixmap(map->pixmap.width() + borderWidth * 2, map->pixmap.height() + borderHeight * 2);
        QPainter borderPainter(&newPixmap);
        for (int y = borderDistance - borderVertDist; y < map->getHeight() + borderVertDist * 2; y += map->getBorderHeight()) {
            for (int x = borderDistance - borderHorzDist; x < map->getWidth() + borderHorzDist * 2; x += map->getBorderWidth()) {
                borderPainter.drawPixmap(x * 16, y * 16, map->layout->border_pixmap);
            }
        }
        borderPainter.drawImage(borderWidth, borderHeight, pixmap.toImage());
        borderPainter.end();
        pixmap = newPixmap;
    }

    if (!ignoreBorder && (this->settings.showUpConnections || this->settings.showDownConnections || this->settings.showLeftConnections || this->settings.showRightConnections)) {
        // if showing connections, draw on outside of image
        QPainter connectionPainter(&pixmap);
        // TODO: Reading the connections from the editor and not 'map' is incorrect.
        for (auto connectionItem : editor->connection_items) {
            const QString direction = connectionItem->connection->direction();
            if ((this->settings.showUpConnections && direction == "up")
             || (this->settings.showDownConnections && direction == "down")
             || (this->settings.showLeftConnections && direction == "left")
             || (this->settings.showRightConnections && direction == "right"))
                connectionPainter.drawImage(connectionItem->x() + borderWidth, connectionItem->y() + borderHeight,
                                            connectionItem->connection->getPixmap().toImage());
        }
        connectionPainter.end();
    }

    // draw events
    if (this->settings.showObjects || this->settings.showWarps || this->settings.showBGs || this->settings.showTriggers || this->settings.showHealLocations) {
        QPainter eventPainter(&pixmap);
        int pixelOffset = 0;
        if (!ignoreBorder && this->settings.showBorder) {
            pixelOffset = this->mode == ImageExporterMode::Normal ? BORDER_DISTANCE * 16 : STITCH_MODE_BORDER_DISTANCE * 16;
        }
        const QList<Event *> events = map->getAllEvents();
        for (const auto &event : events) {
            Event::Group group = event->getEventGroup();
            if ((this->settings.showObjects && group == Event::Group::Object)
             || (this->settings.showWarps && group == Event::Group::Warp)
             || (this->settings.showBGs && group == Event::Group::Bg)
             || (this->settings.showTriggers && group == Event::Group::Coord)
             || (this->settings.showHealLocations && group == Event::Group::Heal)) {
                editor->project->setEventPixmap(event);
                eventPainter.drawImage(QPoint(event->getPixelX() + pixelOffset, event->getPixelY() + pixelOffset), event->getPixmap().toImage());
            }
        }
        eventPainter.end();
    }

    // draw grid directly onto the pixmap
    // since the last grid lines are outside of the pixmap, add a pixel to the bottom and right
    if (this->settings.showGrid) {
        int addX = 1, addY = 1;
        if (borderHeight) addY = 0;
        if (borderWidth) addX = 0;

        QPixmap newPixmap= QPixmap(pixmap.width() + addX, pixmap.height() + addY);
        QPainter gridPainter(&newPixmap);
        gridPainter.drawImage(QPoint(0, 0), pixmap.toImage());
        for (int x = 0; x < newPixmap.width(); x += 16) {
            gridPainter.drawLine(x, 0, x, newPixmap.height());
        }
        for (int y = 0; y < newPixmap.height(); y += 16) {
            gridPainter.drawLine(0, y, newPixmap.width(), y);
        }
        gridPainter.end();
        pixmap = newPixmap;
    }

    return pixmap;
}

void MapImageExporter::updateShowBorderState() {
    // If any of the Connections settings are enabled then this setting is locked (it's implicitly enabled)
    bool on = (this->settings.showUpConnections || this->settings.showDownConnections || this->settings.showLeftConnections || this->settings.showRightConnections);
    const QSignalBlocker blocker(ui->checkBox_Border);
    ui->checkBox_Border->setChecked(on);
    ui->checkBox_Border->setDisabled(on);
    this->settings.showBorder = on;
}

void MapImageExporter::on_checkBox_Elevation_stateChanged(int state) {
    this->settings.showCollision = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Grid_stateChanged(int state) {
    this->settings.showGrid = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Border_stateChanged(int state) {
    this->settings.showBorder = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Objects_stateChanged(int state) {
    this->settings.showObjects = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Warps_stateChanged(int state) {
    this->settings.showWarps = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_BGs_stateChanged(int state) {
    this->settings.showBGs = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Triggers_stateChanged(int state) {
    this->settings.showTriggers = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_HealLocations_stateChanged(int state) {
    this->settings.showHealLocations = (state == Qt::Checked);
    updatePreview();
}

// Shortcut setting for enabling all events
void MapImageExporter::on_checkBox_AllEvents_stateChanged(int state) {
    bool on = (state == Qt::Checked);

    const QSignalBlocker b_Objects(ui->checkBox_Objects);
    ui->checkBox_Objects->setChecked(on);
    ui->checkBox_Objects->setDisabled(on);
    this->settings.showObjects = on;

    const QSignalBlocker b_Warps(ui->checkBox_Warps);
    ui->checkBox_Warps->setChecked(on);
    ui->checkBox_Warps->setDisabled(on);
    this->settings.showWarps = on;

    const QSignalBlocker b_BGs(ui->checkBox_BGs);
    ui->checkBox_BGs->setChecked(on);
    ui->checkBox_BGs->setDisabled(on);
    this->settings.showBGs = on;

    const QSignalBlocker b_Triggers(ui->checkBox_Triggers);
    ui->checkBox_Triggers->setChecked(on);
    ui->checkBox_Triggers->setDisabled(on);
    this->settings.showTriggers = on;

    const QSignalBlocker b_HealLocations(ui->checkBox_HealLocations);
    ui->checkBox_HealLocations->setChecked(on);
    ui->checkBox_HealLocations->setDisabled(on);
    this->settings.showHealLocations = on;

    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionUp_stateChanged(int state) {
    this->settings.showUpConnections = (state == Qt::Checked);
    updateShowBorderState();
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionDown_stateChanged(int state) {
    this->settings.showDownConnections = (state == Qt::Checked);
    updateShowBorderState();
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionLeft_stateChanged(int state) {
    this->settings.showLeftConnections = (state == Qt::Checked);
    updateShowBorderState();
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionRight_stateChanged(int state) {
    this->settings.showRightConnections = (state == Qt::Checked);
    updateShowBorderState();
    updatePreview();
}

// Shortcut setting for enabling all connection directions
void MapImageExporter::on_checkBox_AllConnections_stateChanged(int state) {
    bool on = (state == Qt::Checked);

    const QSignalBlocker b_Up(ui->checkBox_ConnectionUp);
    ui->checkBox_ConnectionUp->setChecked(on);
    ui->checkBox_ConnectionUp->setDisabled(on);
    this->settings.showUpConnections = on;

    const QSignalBlocker b_Down(ui->checkBox_ConnectionDown);
    ui->checkBox_ConnectionDown->setChecked(on);
    ui->checkBox_ConnectionDown->setDisabled(on);
    this->settings.showDownConnections = on;

    const QSignalBlocker b_Left(ui->checkBox_ConnectionLeft);
    ui->checkBox_ConnectionLeft->setChecked(on);
    ui->checkBox_ConnectionLeft->setDisabled(on);
    this->settings.showLeftConnections = on;

    const QSignalBlocker b_Right(ui->checkBox_ConnectionRight);
    ui->checkBox_ConnectionRight->setChecked(on);
    ui->checkBox_ConnectionRight->setDisabled(on);
    this->settings.showRightConnections = on;

    updateShowBorderState();
    updatePreview();
}

void MapImageExporter::on_checkBox_ActualSize_stateChanged(int state) {
    this->settings.previewActualSize = (state == Qt::Checked);
    if (this->settings.previewActualSize) {
        ui->graphicsView_Preview->resetTransform();
    } else {
        scalePreview();
    }
}

void MapImageExporter::on_pushButton_Reset_pressed() {
    this->settings = {};
    for (auto widget : this->findChildren<QCheckBox *>()) {
        const QSignalBlocker b(widget); // Prevent calls to updatePreview
        widget->setChecked(false);
    }
    ui->spinBox_TimelapseDelay->setValue(this->settings.timelapseDelayMs);
    ui->spinBox_FrameSkip->setValue(this->settings.timelapseSkipAmount);
    updatePreview();
}

void MapImageExporter::on_spinBox_TimelapseDelay_valueChanged(int delayMs) {
    this->settings.timelapseDelayMs = delayMs;
}

void MapImageExporter::on_spinBox_FrameSkip_valueChanged(int skip) {
    this->settings.timelapseSkipAmount = skip;
}
