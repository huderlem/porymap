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



/// Emits a signal when a window gets activated / regains focus
class ActiveWindowFilter : public QObject {
    Q_OBJECT
public:
    ActiveWindowFilter(QObject *parent) : QObject(parent) {}
    virtual ~ActiveWindowFilter() {}
    bool eventFilter(QObject *obj, QEvent *event) override;
signals:
    void activated();
};
