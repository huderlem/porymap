#include "filterchildrenproxymodel.h"

#include <QCollator>

bool FilterChildrenProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (this->hideEmpty && source_parent.row() < 0) // want to hide children
    {
        QModelIndex source_index = sourceModel()->index(source_row, this->filterKeyColumn(), source_parent) ;
        if(source_index.isValid())
        {
            if (!sourceModel()->hasChildren(source_index))
                return false;
        }
    }
    // custom behaviour :
    if(filterRegularExpression().pattern().isEmpty() == false)
    {
        // get source-model index for current row
        QModelIndex source_index = sourceModel()->index(source_row, this->filterKeyColumn(), source_parent) ;
        if(source_index.isValid())
        {
            // if any of children matches the filter, then current index matches the filter as well
            int i, nb = sourceModel()->rowCount(source_index);
            for (i = 0; i < nb; ++i)
            {
                if (filterAcceptsRow(i, source_index))
                {
                    return true;
                }
            }
            // check current index itself
            QString key = sourceModel()->data(source_index, filterRole()).toString();
            QString parentKey = sourceModel()->data(source_parent, filterRole()).toString();
            return key.contains(filterRegularExpression()) || parentKey.contains(filterRegularExpression());
        }
    }
    // parent call for initial behaviour
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

bool NumericSortProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const {
    QVariant l = (source_left.model() ? source_left.model()->data(source_left, sortRole()) : QVariant());
    QVariant r = (source_right.model() ? source_right.model()->data(source_right, sortRole()) : QVariant());

    if (l.canConvert<QString>() && r.canConvert<QString>()) {
        // We need to override lexical comparison of strings to do a numeric sort.
        static QCollator collator;
        collator.setNumericMode(true);
        return collator.compare(l.toString(), r.toString()) < 0;
    }
    return QSortFilterProxyModel::lessThan(source_left, source_right);
}
