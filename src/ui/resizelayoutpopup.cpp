#include "resizelayoutpopup.h"
#include "editor.h"
#include "movablerect.h"
#include "config.h"

#include "ui_resizelayoutpopup.h"

// TODO: put this in a util file or something
extern int roundUp(int, int);

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

void BoundedPixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * item, QWidget *widget) {
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
        return QPointF(roundUp(newPos.x(), 16), roundUp(newPos.y(), 16));
    }
    else
        return QGraphicsItem::itemChange(change, value);
}

/******************************************************************************
    ************************************************************************
 ******************************************************************************/

ResizeLayoutPopup::ResizeLayoutPopup(QWidget *parent, Editor *editor) :
    QDialog(parent),
    parent(parent),
    editor(editor),
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

ResizeLayoutPopup::~ResizeLayoutPopup()
{
    delete ui;
}

/// Reset position of the dialog to cover the MainWindow's layout metatile scene
void ResizeLayoutPopup::resetPosition() {
    this->setGeometry(QRect(parent->mapToGlobal(QPoint(0, 0)), parent->size()));
}

void ResizeLayoutPopup::on_buttonBox_clicked(QAbstractButton *button) {
    if(button == this->ui->buttonBox->button(QDialogButtonBox::Reset) ) {
        this->scene->clear();
        setupLayoutView();
    }
}

/// Custom scene contains
///   (1) pixmap representing the current layout / not resizable / drag-movable
///   (1) layout outline / resizable / not movable
void ResizeLayoutPopup::setupLayoutView() {
    if (!this->editor || !this->editor->layout) return;

    // Border stuff
    bool bordersEnabled = projectConfig.useCustomBorderSize;
    if (bordersEnabled) {
        this->ui->spinBox_borderWidth->setMinimum(1);
        this->ui->spinBox_borderHeight->setMinimum(1);
        this->ui->spinBox_borderWidth->setMaximum(MAX_BORDER_WIDTH);
        this->ui->spinBox_borderHeight->setMaximum(MAX_BORDER_HEIGHT);
        this->ui->spinBox_borderWidth->setLineEditEnabled(false);
        this->ui->spinBox_borderHeight->setLineEditEnabled(false);
    } else {
        this->ui->frame_border->setVisible(false);
    }
    this->ui->spinBox_borderWidth->setValue(this->editor->layout->getBorderWidth());
    this->ui->spinBox_borderHeight->setValue(this->editor->layout->getBorderHeight());

    // Layout stuff
    QPixmap pixmap = this->editor->layout->pixmap;
    this->layoutPixmap = new BoundedPixmapItem(pixmap);
    this->scene->addItem(layoutPixmap);
    int maxWidth = this->editor->project->getMaxMapWidth();
    int maxHeight = this->editor->project->getMaxMapHeight();
    QGraphicsRectItem *cover = new QGraphicsRectItem(-maxWidth * 8, -maxHeight * 8, maxWidth * 16, maxHeight * 16);
    this->scene->addItem(cover);

    this->ui->spinBox_width->setMinimum(1);
    this->ui->spinBox_width->setMaximum(maxWidth);
    this->ui->spinBox_height->setMinimum(1);
    this->ui->spinBox_height->setMaximum(maxHeight);

    this->ui->spinBox_width->setLineEditEnabled(false);
    this->ui->spinBox_height->setLineEditEnabled(false);

    static bool layoutSizeRectVisible = true;

    this->outline = new ResizableRect(this, &layoutSizeRectVisible, this->editor->layout->getWidth(), this->editor->layout->getHeight(), qRgb(255, 0, 255));
    this->outline->setLimit(cover->rect().toAlignedRect());
    connect(outline, &ResizableRect::rectUpdated, [=](QRect rect){
        this->scene->setValidRect(rect);
        this->ui->spinBox_width->setValue(rect.width() / 16);
        this->ui->spinBox_height->setValue(rect.height() / 16);
    });
    scene->addItem(outline);

    layoutPixmap->setBoundary(outline);
    this->outline->rectUpdated(outline->rect().toAlignedRect());

    // TODO: is this an ideal size for all maps, or should this adjust based on starting dimensions?
    this->ui->graphicsView->setTransform(QTransform::fromScale(0.5, 0.5));
    this->ui->graphicsView->centerOn(layoutPixmap);
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
