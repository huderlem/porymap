#ifndef FILTERCHILDRENPROXYMODEL_H
#define FILTERCHILDRENPROXYMODEL_H

#include <QSortFilterProxyModel>

class FilterChildrenProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit FilterChildrenProxyModel(QObject *parent = nullptr);
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const;

};

#endif // FILTERCHILDRENPROXYMODEL_H
