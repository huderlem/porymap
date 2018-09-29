#ifndef MAPSCENEEVENTFILTER_H
#define MAPSCENEEVENTFILTER_H

#include <QObject>

class MapSceneEventFilter : public QObject
{
    Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
public:
    explicit MapSceneEventFilter(QObject *parent = nullptr);

signals:
    void wheelZoom(int delta);
public slots:
};

#endif // MAPSCENEEVENTFILTER_H
