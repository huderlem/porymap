#include "resizelayoutpopup.h"
#include "editor.h"
#include "movablerect.h"
#include "config.h"
#include "utility.h"
#include "message.h"

#include "ui_resizelayoutpopup.h"

CheckeredBgScene::CheckeredBgScene(QObject *parent) : QGraphicsScene(parent) { }

void CheckeredBgScene::drawBackground(QPainter *painter, const QRectF &rect) {
    QRect r = rect.toRect();
    int xMin = r.left() - r.left() % this->gridSize - this->gridSize;
    int yMin = r.top() - r.top() % this->gridSize - this->gridSize;
    int xMax = r.right() - r.right() % this->gridSize + this->gridSize;
    int yMax = r.bottom() - r.bottom() % this->gridSize + this->gridSize;

    // draw grid 16x16 from top to bottom of scene
    QColor paintColor(0x00ff00);
    for (int x = xMin, xTile = 0; x <= xMax; x += this->gridSize, xTile++) {
        for (int y = yMin, yTile = 0; y <= yMax; y += this->gridSize, yTile++) {
            if (!((xTile ^ yTile) & 1)) { // tile numbers have same parity (evenness)
                if (this->validRect.contains(x, y))
                    paintColor = QColor(132, 217, 165); // green light color
                else
                    paintColor = 0xbcbcbc; // normal light color
            }
            else {
                if (this->validRect.contains(x, y))
                    paintColor = QColor(76, 178, 121); // green dark color
                else
                    paintColor = 0x969696; // normal dark color
            }
            painter->fillRect(QRect(x, y, this->gridSize, this->gridSize), paintColor);
        }
    }
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

BoundedPixmapItem::BoundedPixmapItem(const QPixmap &pixmap, QGraphicsItem *parent) : QGraphicsPixmapItem(pixmap, parent) {
    setFlags(this->flags() | QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemSendsGeometryChanges | QGraphicsItem::ItemIsSelectable);
}

void BoundedPixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    // Draw the pixmap darkened in the background
    painter->fillRect(this->boundingRect().toAlignedRect(), QColor(0x444444));
    painter->setCompositionMode(QPainter::CompositionMode_Multiply);
    painter->drawPixmap(this->boundingRect().toAlignedRect(), this->pixmap());

    // draw the normal pixmap on top, cropping to validRect as needed
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    QRect intersection = this->mapRectFromScene(this->boundary->rect()).toAlignedRect() & this->boundingRect().toAlignedRect();
    QPixmap cropped = this->pixmap().copy(intersection);
    painter->drawPixmap(intersection, cropped);
}

QVariant BoundedPixmapItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();
        return QPointF(Util::roundUp(newPos.x(), 16), Util::roundUp(newPos.y(), 16));
    }
    else
        return QGraphicsItem::itemChange(change, value);
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

ResizeLayoutPopup::ResizeLayoutPopup(QWidget *parent, Layout *layout, Project *project) :
    QDialog(parent),
    parent(parent),
    layout(layout),
    project(project),
    ui(new Ui::ResizeLayoutPopup)
{
    ui->setupUi(this);
    this->resetPosition();
    this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    this->setWindowModality(Qt::ApplicationModal);

    this->scene = new CheckeredBgScene(this);
    this->ui->graphicsView->setScene(this->scene);
    this->ui->graphicsView->setRenderHints(QPainter::Antialiasing);
    this->ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
}

ResizeLayoutPopup::~ResizeLayoutPopup() {
    delete ui;
}

/// Reset position of the dialog to cover the MainWindow's layout metatile scene
void ResizeLayoutPopup::resetPosition() {
    this->setGeometry(QRect(parent->mapToGlobal(QPoint(0, 0)), parent->size()));
}

void ResizeLayoutPopup::on_buttonBox_clicked(QAbstractButton *button) {
    if (button == this->ui->buttonBox->button(QDialogButtonBox::Reset) ) {
        this->scene->clear();
        setupLayoutView();
    }
}

/// Custom scene contains
///   (1) pixmap representing the current layout / not resizable / drag-movable
///   (1) layout outline / resizable / not movable
void ResizeLayoutPopup::setupLayoutView() {
    if (!this->layout || !this->project) return;

    // Border stuff
    if (projectConfig.useCustomBorderSize) {
        this->ui->spinBox_borderWidth->setMinimum(1);
        this->ui->spinBox_borderHeight->setMinimum(1);
        this->ui->spinBox_borderWidth->setMaximum(MAX_BORDER_WIDTH);
        this->ui->spinBox_borderHeight->setMaximum(MAX_BORDER_HEIGHT);
    } else {
        this->ui->frame_border->setVisible(false);
    }
    this->ui->spinBox_borderWidth->setValue(this->layout->getBorderWidth());
    this->ui->spinBox_borderHeight->setValue(this->layout->getBorderHeight());

    // Layout stuff
    this->layoutPixmap = new BoundedPixmapItem(this->layout->pixmap);
    this->scene->addItem(layoutPixmap);
    int maxWidth = this->project->getMaxMapWidth();
    int maxHeight = this->project->getMaxMapHeight();
    QGraphicsRectItem *cover = new QGraphicsRectItem(-maxWidth * 8, -maxHeight * 8, maxWidth * 16, maxHeight * 16);
    this->scene->addItem(cover);

    this->ui->spinBox_width->setMinimum(1);
    this->ui->spinBox_width->setMaximum(maxWidth);
    this->ui->spinBox_height->setMinimum(1);
    this->ui->spinBox_height->setMaximum(maxHeight);

    static bool layoutSizeRectVisible = true;

    this->outline = new ResizableRect(this, &layoutSizeRectVisible, this->layout->getWidth(), this->layout->getHeight(), qRgb(255, 0, 255));
    this->outline->setZValue(Editor::ZValue::ResizeLayoutPopup); // Ensure on top of view
    this->outline->setLimit(cover->rect().toAlignedRect());
    connect(outline, &ResizableRect::rectUpdated, [=](QRect rect){
        // Note: this extra limit check needs access to the project values, so it is done here and not ResizableRect::mouseMoveEvent
        int size = this->project->getMapDataSize(rect.width() / 16, rect.height() / 16);
        int maxSize = this->project->getMaxMapDataSize();
        if (size > maxSize) {
            QSize addition = this->project->getMapSizeAddition();
            WarningMessage::show(QStringLiteral("The specified width and height are too large."),
                                 QString("The maximum layout width and height is the following: (width + %1) * (height + %2) <= %3\n"
                                         "The specified layout width and height was: (%4 + %1) * (%5 + %2) = %6")
                                            .arg(addition.width())
                                            .arg(addition.height())
                                            .arg(maxSize)
                                            .arg(rect.width() / 16)
                                            .arg(rect.height() / 16)
                                            .arg(size),
                                this);
            // adjust rect to last accepted size
            rect = this->scene->getValidRect();
        }
        this->scene->setValidRect(rect);
        this->outline->setRect(rect);
        this->ui->spinBox_width->setValue(rect.width() / 16);
        this->ui->spinBox_height->setValue(rect.height() / 16);
    });
    scene->addItem(outline);

    layoutPixmap->setBoundary(outline);
    emit this->outline->rectUpdated(outline->rect().toAlignedRect());

    this->scale = 1.0;

    QRectF rect = this->outline->rect();
    const int marginSize = 10 * 16; // Leave a margin of 10 metatiles around the map
    rect += QMargins(marginSize, marginSize, marginSize, marginSize);
    this->ui->graphicsView->fitInView(rect, Qt::KeepAspectRatio);
}

void ResizeLayoutPopup::on_spinBox_width_valueChanged(int value) {
    if (!this->outline) return;
    QRectF rect = this->outline->rect();
    this->outline->updatePosFromRect(QRect(rect.x(), rect.y(), value * 16, rect.height()));
}

void ResizeLayoutPopup::on_spinBox_height_valueChanged(int value) {
    if (!this->outline) return;
    QRectF rect = this->outline->rect();
    this->outline->updatePosFromRect(QRect(rect.x(), rect.y(), rect.width(), value * 16));
}

/// Result is the number of metatiles to add (or subtract) to each side of the map after dimension changes
QMargins ResizeLayoutPopup::getResult() {
    QMargins result = QMargins();

    result.setLeft(this->layoutPixmap->x() - this->outline->rect().left());
    result.setTop(this->layoutPixmap->y() - this->outline->rect().top());
    result.setRight(this->outline->rect().right() - (this->layoutPixmap->x() + this->layoutPixmap->pixmap().width()));
    result.setBottom(this->outline->rect().bottom() - (this->layoutPixmap->y() + this->layoutPixmap->pixmap().height()));

    return result / 16;
}

QSize ResizeLayoutPopup::getBorderResult() {
    return QSize(this->ui->spinBox_borderWidth->value(), this->ui->spinBox_borderHeight->value());
}

void ResizeLayoutPopup::zoom(qreal delta) {
    if (this->scale + delta < 0.05 || this->scale + delta > 3.0)
        return;
    this->scale += delta;
    this->ui->graphicsView->scale(1.0 + delta, 1.0 + delta);
}

void ResizeLayoutPopup::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        zoom(+0.1);
    } else if (event->key() == Qt::Key_Minus || event->key() == Qt::Key_Underscore) {
        zoom(-0.1);
    } else {
        QDialog::keyPressEvent(event);
    }
}
