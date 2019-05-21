#include "mapimageexporter.h"
#include "ui_mapimageexporter.h"

#include <QFileDialog>
#include <QMatrix>
#include <QImage>
#include <QPainter>
#include <QPoint>

MapImageExporter::MapImageExporter(QWidget *parent_, Editor *editor_) :
    QDialog(parent_),
    ui(new Ui::MapImageExporter)
{
    ui->setupUi(this);
    this->map = editor_->map;
    this->editor = editor_;

    this->ui->comboBox_MapSelection->addItems(*editor->project->mapNames);
    this->ui->comboBox_MapSelection->setCurrentText(map->name);
    this->ui->comboBox_MapSelection->setEnabled(false);// TODO: allow selecting map from drop-down

    updatePreview();
}

MapImageExporter::~MapImageExporter() {
    delete ui;
}

void MapImageExporter::saveImage() {
    QString defaultFilepath = QString("%1/%2.png").arg(editor->project->root).arg(map->name);
    QString filepath = QFileDialog::getSaveFileName(this, "Export Map Image", defaultFilepath,
                                                    "Image Files (*.png *.jpg *.bmp)");
    if (!filepath.isEmpty()) {
        this->ui->graphicsView_Preview->grab(this->ui->graphicsView_Preview->sceneRect().toRect()).save(filepath);
        this->close();
    }
}

void MapImageExporter::updatePreview() {
    if (scene) {
        delete scene;
        scene = nullptr;
    }
    scene = new QGraphicsScene;

    // draw background layer / base image
    if (showCollision) {
        preview = map->collision_pixmap;
    } else {
        preview = map->pixmap;
    }

    // draw events
    QPainter eventPainter(&preview);
    QList<Event*> events = map->getAllEvents();
    for (Event *event : events) {
        QString group = event->get("event_group_type");
        if ((showObjects && group == "object_event_group")
         || (showWarps && group == "warp_event_group")
         || (showBGs && group == "bg_event_group")
         || (showTriggers && group == "coord_event_group")
         || (showHealSpots && group == "heal_event_group"))
            eventPainter.drawImage(QPoint(event->getPixelX(), event->getPixelY()), event->pixmap.toImage());
    }
    eventPainter.end();

    // draw map border
    // note: this will break when allowing map to be selected from drop down maybe
    int borderHeight = 0, borderWidth = 0;
    if (showUpConnections || showDownConnections || showLeftConnections || showRightConnections) showBorder = true;
    if (showBorder) {
        borderHeight = 32 * 3, borderWidth = 32 * 3;
        QPixmap newPreview = QPixmap(map->pixmap.width() + borderWidth * 2, map->pixmap.height() + borderHeight * 2);
        QPainter borderPainter(&newPreview);
        for (auto borderItem : editor->borderItems) {
            borderPainter.drawImage(QPoint(borderItem->x() + borderWidth, borderItem->y() + borderHeight), 
                                    borderItem->pixmap().toImage());
        }
        borderPainter.drawImage(QPoint(borderWidth, borderHeight), preview.toImage());
        borderPainter.end();
        preview = newPreview;
    }

    // if showing connections, draw on outside of image
    QPainter connectionPainter(&preview);
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

    // draw grid directly onto the pixmap
    // since the last grid lines are outside of the pixmap, add a pixel to the bottom and right
    if (showGrid) {
        int addX = 1, addY = 1;
        if (borderHeight) addY = 0;
        if (borderWidth) addX = 0;

        QPixmap newPreview = QPixmap(preview.width() + addX, preview.height() + addY);
        QPainter gridPainter(&newPreview);
        gridPainter.drawImage(QPoint(0, 0), preview.toImage());
        for (auto lineItem : editor->gridLines) {
            QPointF addPos(borderWidth, borderHeight);
            gridPainter.drawLine(lineItem->line().p1() + addPos, lineItem->line().p2() + addPos);
        }
        gridPainter.end();
        preview = newPreview;
    }

    scene->addPixmap(preview);

    this->scene->setSceneRect(this->scene->itemsBoundingRect());

    this->ui->graphicsView_Preview->setScene(scene);
    this->ui->graphicsView_Preview->setFixedSize(scene->itemsBoundingRect().width() + 2,
                                                 scene->itemsBoundingRect().height() + 2);
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
