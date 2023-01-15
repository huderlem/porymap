#pragma once
#ifndef ENCOUNTERTABLEDELEGATES_H
#define ENCOUNTERTABLEDELEGATES_H

#include <QStyledItemDelegate>
#include <QStringList>



class Project;

class SpeciesComboDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    SpeciesComboDelegate(Project *project, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    Project *project = nullptr;
};



class SpinBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    SpinBoxDelegate(Project *project, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    Project *project = nullptr;
};

#endif // ENCOUNTERTABLEDELEGATES_H
