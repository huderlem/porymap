#ifndef WILDMONSEARCH_H
#define WILDMONSEARCH_H

#include <QDialog>

class Project;

namespace Ui {
class WildMonSearch;
}

class WildMonSearch : public QDialog
{
    Q_OBJECT

public:
    explicit WildMonSearch(Project *project, QWidget *parent = nullptr);
    ~WildMonSearch();

    void refresh();

signals:
    void openWildMonTableRequested(const QString &mapName, const QString &groupName, const QString &fieldName);

private:
    struct RowData {
        QString mapName;
        QString groupName;
        QString fieldName;
        QString levelRange;
        QString chance;
    };

    Ui::WildMonSearch *ui;
    Project *const project;
    QMap<QString,QMap<int,QString>> percentageStrings;
    QMap<QString,QList<RowData>> resultsCache;

    void addTableEntry(const RowData &rowData);
    QList<RowData> search(const QString &species) const;
    void updatePercentageStrings();
    void updateResults(const QString &species);
    void cellDoubleClicked(int row, int column);

};

#endif // WILDMONSEARCH_H
