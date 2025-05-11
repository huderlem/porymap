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

    void setEditsAllowedButtonVisible(bool visible);
    void setEditsAllowed(bool allowed);
    void toggleEditsAllowed();

    void setEmptyFoldersVisible(bool visible);
    void toggleEmptyFolders();

    void expandList();
    void collapseList();

    void applyFilter(const QString &filterText);
    void clearFilter();
    void setFilterLocked(bool locked) { m_filterLocked = locked; }
    bool isFilterLocked() const { return m_filterLocked; }

    void setExpandListForSearch(bool expand) { m_expandListForSearch = expand; }
    bool getExpandListForSearch() const { return m_expandListForSearch; }

    void setSearchFocus();

signals:
    void filterCleared(MapTree*);
    void addFolderClicked();
    void editsAllowedChanged(bool allowed);
    void emptyFoldersVisibleChanged(bool visible);

private:
    Ui::MapListToolBar *ui;
    QPointer<MapTree> m_list;
    bool m_filterLocked = false;
    bool m_editsAllowed = false;
    bool m_emptyFoldersVisible = true;
    bool m_expandListForSearch = true;
};

#endif // MAPLISTTOOLBAR_H
