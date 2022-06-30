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

MapImageExporter::MapImageExporter(QWidget *parent_, Editor *editor_, ImageExporterMode mode) :
    QDialog(parent_),
    ui(new Ui::MapImageExporter)
{
    ui->setupUi(this);
    this->map = editor_->map;
    this->editor = editor_;
    this->mode = mode;
    this->setWindowTitle(getTitle(this->mode));
    this->ui->groupBox_Connections->setVisible(this->mode == ImageExporterMode::Normal);
    this->ui->groupBox_Timelapse->setVisible(this->mode == ImageExporterMode::Timelapse);

    this->ui->comboBox_MapSelection->addItems(editor->project->mapNames);
    this->ui->comboBox_MapSelection->setCurrentText(map->name);
    this->ui->comboBox_MapSelection->setEnabled(false);// TODO: allow selecting map from drop-down

    updatePreview();
}

MapImageExporter::~MapImageExporter() {
    delete scene;
    delete ui;
}

void MapImageExporter::saveImage() {
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
            .arg(editor->project->root)
            .arg(defaultFilename)
            .arg(this->mode == ImageExporterMode::Timelapse ? "gif" : "png");
    QString filter = this->mode == ImageExporterMode::Timelapse ? "Image Files (*.gif)" : "Image Files (*.png *.jpg *.bmp)";
    QString filepath = QFileDialog::getSaveFileName(this, title, defaultFilepath, filter);
    if (!filepath.isEmpty()) {
        switch (this->mode) {
            case ImageExporterMode::Normal:
                this->preview.save(filepath);
                break;
        case ImageExporterMode::Stitch: {
                QProgressDialog progress("Building map stitch...", "Cancel", 0, 1, this);
                progress.setAutoClose(true);
                progress.setWindowModality(Qt::WindowModal);
                progress.setModal(true);
                QPixmap pixmap = this->getStitchedImage(&progress, this->showBorder);
                if (progress.wasCanceled()) {
                    progress.close();
                    return;
                }
                pixmap.save(filepath);
                progress.close();
                break;
            }
            case ImageExporterMode::Timelapse:
                QProgressDialog progress("Building map timelapse...", "Cancel", 0, 1, this);
                progress.setAutoClose(true);
                progress.setWindowModality(Qt::WindowModal);
                progress.setModal(true);
                progress.setMaximum(1);
                progress.setValue(0);

                int maxWidth = this->map->getWidth() * 16;
                int maxHeight = this->map->getHeight() * 16;
                if (showBorder) {
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
                    if (showBorder) {
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
                timelapseImg.setDefaultDelay(timelapseDelayMs);
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
                    QPixmap pixmap = this->getFormattedMapPixmap(this->map, !this->showBorder);
                    if (pixmap.width() < maxWidth || pixmap.height() < maxHeight) {
                        QPixmap pixmap2 = QPixmap(maxWidth, maxHeight);
                        QPainter painter(&pixmap2);
                        pixmap2.fill(QColor(0, 0, 0));
                        painter.drawPixmap(0, 0, pixmap.width(), pixmap.height(), pixmap);
                        painter.end();
                        pixmap = pixmap2;
                    }
                    timelapseImg.addFrame(pixmap.toImage());
                    for (int j = 0; j < timelapseSkipAmount; j++) {
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
                QPixmap pixmap = this->getFormattedMapPixmap(this->map, !this->showBorder);
                timelapseImg.addFrame(pixmap.toImage());
                timelapseImg.save(filepath);
                progress.close();
                break;
        }
        this->close();
    }
}

bool MapImageExporter::historyItemAppliesToFrame(const QUndoCommand *command) {
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
            return this->showCollision;
        case CommandId::ID_PaintBorder:
            return this->showBorder;
        case CommandId::ID_EventMove:
        case CommandId::ID_EventShift:
        case CommandId::ID_EventCreate:
        case CommandId::ID_EventDelete:
        case CommandId::ID_EventDuplicate: {
            bool eventTypeIsApplicable =
                       (this->showObjects   && (command->id() & IDMask_EventType_Object)  != 0)
                    || (this->showWarps     && (command->id() & IDMask_EventType_Warp)    != 0)
                    || (this->showBGs       && (command->id() & IDMask_EventType_BG)      != 0)
                    || (this->showTriggers  && (command->id() & IDMask_EventType_Trigger) != 0)
                    || (this->showHealSpots && (command->id() & IDMask_EventType_Heal)    != 0);
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

        for (MapConnection *connection : cur.map->connections) {
            if (connection->direction == "dive" || connection->direction == "emerge")
                continue;
            int x = cur.x;
            int y = cur.y;
            int offset = connection->offset.toInt(nullptr, 0);
            Map *connectionMap = this->editor->project->loadMap(connection->map_name);
            if (connection->direction == "up") {
                x += offset;
                y -= connectionMap->getHeight();
            } else if (connection->direction == "down") {
                x += offset;
                y += cur.map->getHeight();
            } else if (connection->direction == "left") {
                x -= connectionMap->getWidth();
                y += offset;
            } else if (connection->direction == "right") {
                x += cur.map->getWidth();
                y += offset;
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
        QPixmap pixmap = this->getFormattedMapPixmap(map.map, false);
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
    if (scene) {
        delete scene;
        scene = nullptr;
    }

    preview = getFormattedMapPixmap(this->map, false);
    scene = new QGraphicsScene;
    scene->addPixmap(preview);
    this->scene->setSceneRect(this->scene->itemsBoundingRect());

    this->ui->graphicsView_Preview->setScene(scene);
    this->ui->graphicsView_Preview->setFixedSize(scene->itemsBoundingRect().width() + 2,
                                                 scene->itemsBoundingRect().height() + 2);
}

QPixmap MapImageExporter::getFormattedMapPixmap(Map *map, bool ignoreBorder) {
    QPixmap pixmap;

    // draw background layer / base image
    map->render(true);
    if (showCollision) {
        map->renderCollision(editor->collisionOpacity, true);
        pixmap = map->collision_pixmap;
    } else {
        pixmap = map->pixmap;
    }

    // draw events
    QPainter eventPainter(&pixmap);
    QList<Event*> events = map->getAllEvents();
    for (Event *event : events) {
        editor->project->setEventPixmap(event);
        QString group = event->get("event_group_type");
        if ((showObjects && group == EventGroup::Object)
         || (showWarps && group == EventGroup::Warp)
         || (showBGs && group == EventGroup::Bg)
         || (showTriggers && group == EventGroup::Coord)
         || (showHealSpots && group == EventGroup::Heal))
            eventPainter.drawImage(QPoint(event->getPixelX(), event->getPixelY()), event->pixmap.toImage());
    }
    eventPainter.end();

    // draw map border
    // note: this will break when allowing map to be selected from drop down maybe
    int borderHeight = 0, borderWidth = 0;
    bool forceDrawBorder = showUpConnections || showDownConnections || showLeftConnections || showRightConnections;
    if (!ignoreBorder && (showBorder || forceDrawBorder)) {
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

    if (!this->mode) {
        // if showing connections, draw on outside of image
        QPainter connectionPainter(&pixmap);
        for (auto connectionItem : editor->connection_edit_items) {
            QString direction = connectionItem->connection->direction;
            if ((showUpConnections && direction == "up")
             || (showDownConnections && direction == "down")
             || (showLeftConnections && direction == "left")
             || (showRightConnections && direction == "right"))
                connectionPainter.drawImage(connectionItem->initialX + borderWidth, connectionItem->initialY + borderHeight,
                                            connectionItem->basePixmap.toImage());
        }
        connectionPainter.end();
    }

    // draw grid directly onto the pixmap
    // since the last grid lines are outside of the pixmap, add a pixel to the bottom and right
    if (showGrid) {
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

void MapImageExporter::on_checkBox_Elevation_stateChanged(int state) {
    showCollision = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Grid_stateChanged(int state) {
    showGrid = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Border_stateChanged(int state) {
    showBorder = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Objects_stateChanged(int state) {
    showObjects = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Warps_stateChanged(int state) {
    showWarps = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_BGs_stateChanged(int state) {
    showBGs = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_Triggers_stateChanged(int state) {
    showTriggers = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_HealSpots_stateChanged(int state) {
    showHealSpots = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionUp_stateChanged(int state) {
    showUpConnections = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionDown_stateChanged(int state) {
    showDownConnections = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionLeft_stateChanged(int state) {
    showLeftConnections = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_checkBox_ConnectionRight_stateChanged(int state) {
    showRightConnections = (state == Qt::Checked);
    updatePreview();
}

void MapImageExporter::on_pushButton_Save_pressed() {
    saveImage();
}

void MapImageExporter::on_pushButton_Reset_pressed() {
    for (auto widget : this->findChildren<QCheckBox *>())
        widget->setChecked(false);
}

void MapImageExporter::on_pushButton_Cancel_pressed() {
    this->close();
}

void MapImageExporter::on_spinBox_TimelapseDelay_valueChanged(int delayMs) {
    timelapseDelayMs = delayMs;
}

void MapImageExporter::on_spinBox_FrameSkip_valueChanged(int skip) {
    timelapseSkipAmount = skip;
}
