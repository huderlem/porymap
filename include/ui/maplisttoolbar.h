#ifndef MAPLISTTOOLBAR_H
#define MAPLISTTOOLBAR_H

#include "maplistmodels.h"
#include "filterchildrenproxymodel.h"

#include <QFrame>
#include <QPointer>

namespace Ui {
class MapListToolBar;
}

class MapListToolBar : public QFrame
{
    Q_OBJECT

public:
    explicit MapListToolBar(QWidget *parent = nullptr);
    ~MapListToolBar();

    MapTree* list() const { return m_list; }
    void setList(MapTree *list);

    void setEditsAllowedButtonHidden(bool hidden);

    void toggleEmptyFolders();
    void expandList();
    void collapseList();
    void toggleEditsAllowed();

    void applyFilter(const QString &filterText);
    void clearFilter();
    void setFilterLocked(bool locked) { m_filterLocked = locked; }
    bool isFilterLocked() const { return m_filterLocked; }

signals:
    void filterCleared(MapTree*);
    void addFolderClicked();

private:
    Ui::MapListToolBar *ui;
    QPointer<MapTree> m_list;
    bool m_filterLocked = false;

    void setEditsAllowed(bool allowed);
};

#endif // MAPLISTTOOLBAR_H
