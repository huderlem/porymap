#include <QObject>
#include <QEvent>



class WheelFilter : public QObject {
    Q_OBJECT
public:
    WheelFilter(QObject *parent) : QObject(parent) {}
    virtual ~WheelFilter() {}
    bool eventFilter(QObject *obj, QEvent *event) override;
};
