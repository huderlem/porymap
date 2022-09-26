#ifndef PREFABFRAME_H
#define PREFABFRAME_H

#include <QFrame>

namespace Ui {
class PrefabFrame;
}

class PrefabFrame : public QFrame
{
    Q_OBJECT

public:
    explicit PrefabFrame(QWidget *parent = nullptr);
    ~PrefabFrame();

public:
    Ui::PrefabFrame *ui;
};

#endif // PREFABFRAME_H
