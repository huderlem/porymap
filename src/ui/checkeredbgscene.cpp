#include "checkeredbgscene.h"

#include <QPainter>

void CheckeredBgScene::drawBackground(QPainter *painter, const QRectF &rect) {
    QRect r = rect.toRect();
    int w = this->gridSize.width();
    int h = this->gridSize.height();
    int xMin = r.left() - (r.left() % w) - w;
    int yMin = r.top() - (r.top() % h) - h;
    int xMax = r.right() - (r.right() % w) + w;
    int yMax = r.bottom() - (r.bottom() % h) + h;

    // draw grid from top to bottom of scene
    QColor paintColor;
    for (int x = xMin; x <= xMax; x += w) {
        for (int y = yMin; y <= yMax; y += h) {
            if ((x/w + y/h) % 2) {
                if (this->validRect.contains(x, y))
                    paintColor = QColor(132, 217, 165); // green light color
                else
                    paintColor = 0xbcbcbc; // normal light color
            } else {
                if (this->validRect.contains(x, y))
                    paintColor = QColor(76, 178, 121); // green dark color
                else
                    paintColor = 0x969696; // normal dark color
            }
            painter->fillRect(QRect(x, y, w, h), paintColor);
        }
    }
}
