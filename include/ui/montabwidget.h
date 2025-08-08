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
    ~MonTabWidget();

    void populate();
    void populateTab(int tabIndex, WildMonInfo monInfo);
    void clear();

    void clearTableAt(int index);

    QTableView *tableAt(int tabIndex);

    void copy(int index);
    void paste(int index);

    void setCurrentField(const QString &fieldName);

public slots:
    void setTabActive(int index, bool active = true);
    void deactivateTab(int tabIndex);

signals:
    void edited();

private:
    void actionCopyTab(int index);
    void actionAddDeleteTab(int index);

    QVector<bool> activeTabs;
    QVector<QPushButton *> addDeleteTabButtons;
    QVector<QPushButton *> copyTabButtons;
    QMap<QString,int> fieldNameToIndex;

    Editor *editor;
};

#endif // MONTABWIDGET_H
