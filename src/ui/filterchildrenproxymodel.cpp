#include "filterchildrenproxymodel.h"

FilterChildrenProxyModel::FilterChildrenProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{

}

bool FilterChildrenProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
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
