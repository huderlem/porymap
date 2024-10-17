#include <QObject>
#include <QEvent>



/// Prevent wheel scroll
class WheelFilter : public QObject {
    Q_OBJECT
public:
    WheelFilter(QObject *parent) : QObject(parent) {}
    virtual ~WheelFilter() {}
    bool eventFilter(QObject *obj, QEvent *event) override;
};



/// Ctrl+Wheel = zoom
class MapSceneEventFilter : public QObject {
    Q_OBJECT
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
public:
    explicit MapSceneEventFilter(QObject *parent = nullptr) : QObject(parent) {}

signals:
    void wheelZoom(int delta);
public slots:
};
