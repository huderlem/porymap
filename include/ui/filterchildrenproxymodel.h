#ifndef FILTERCHILDRENPROXYMODEL_H
#define FILTERCHILDRENPROXYMODEL_H

#include <QSortFilterProxyModel>

class NumericSortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit NumericSortProxyModel(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {};

protected:
    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
};

class FilterChildrenProxyModel : public NumericSortProxyModel
{
    Q_OBJECT

public:
    explicit FilterChildrenProxyModel(QObject *parent = nullptr) : NumericSortProxyModel(parent) {};
    void setHideEmpty(bool hidden) { this->hideEmpty = hidden; }

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;

private:
    bool hideEmpty = false;
};

#endif // FILTERCHILDRENPROXYMODEL_H
