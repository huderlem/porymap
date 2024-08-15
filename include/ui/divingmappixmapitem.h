#ifndef DIVINGMAPPIXMAPITEM_H
#define DIVINGMAPPIXMAPITEM_H

#include "mapconnection.h"

#include <QGraphicsPixmapItem>
#include <QPointer>
#include <QComboBox>

class DivingMapPixmapItem : public QObject, public QGraphicsPixmapItem {
    Q_OBJECT
public:
    DivingMapPixmapItem(MapConnection *connection, QComboBox *combo);
    ~DivingMapPixmapItem();

    MapConnection* connection() const { return m_connection; }
    void updatePixmap();

private:
    QPointer<MapConnection> m_connection;
    QPointer<QComboBox> m_combo;

    void setComboText(const QString &text);
    static QPixmap getBasePixmap(MapConnection* connection);

private slots:
    void onTargetMapChanged();
};

#endif // DIVINGMAPPIXMAPITEM_H
