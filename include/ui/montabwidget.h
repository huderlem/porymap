#ifndef MONTABWIDGET_H
#define MONTABWIDGET_H

#include "wildmoninfo.h"

#include <QtWidgets>
#include <QVector>

class Editor;

class MonTabWidget : public QTabWidget {

    Q_OBJECT

public:
    explicit MonTabWidget(Editor *editor = nullptr, QWidget *parent = nullptr);
    ~MonTabWidget(){};

    void populate();
    void populateTab(int tabIndex, WildMonInfo monInfo);
    void clear();

    void clearTableAt(int index);

    QTableView *tableAt(int tabIndex);

public slots:
    void setTabActive(int index, bool active = true);

private:
    bool eventFilter(QObject *object, QEvent *event);
    void askActivateTab(int tabIndex, QPoint menuPos);

    QVector<bool> activeTabs;

    Editor *editor;
};

#endif // MONTABWIDGET_H
