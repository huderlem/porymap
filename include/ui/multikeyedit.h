#ifndef MULTIKEYEDIT_H
#define MULTIKEYEDIT_H

#include <QWidget>
#include <QKeySequenceEdit>

class QLineEdit;


// A collection of QKeySequenceEdit's laid out horizontally.
class MultiKeyEdit : public QWidget
{
    Q_OBJECT

public:
    MultiKeyEdit(QWidget *parent = nullptr, int fieldCount = 2);

    bool eventFilter(QObject *watched, QEvent *event) override;

    int fieldCount() const;
    void setFieldCount(int count);
    QList<QKeySequence> keySequences() const;
    bool removeOne(const QKeySequence &keySequence);
    bool contains(const QKeySequence &keySequence) const;
    void setContextMenuPolicy(Qt::ContextMenuPolicy policy);
    bool isClearButtonEnabled() const;
    void setClearButtonEnabled(bool enable);

public slots:
    void clear();
    void setKeySequences(const QList<QKeySequence> &keySequences);
    void addKeySequence(const QKeySequence &keySequence);

signals:
    void keySequenceChanged(const QKeySequence &keySequence);
    void editingFinished();
    void customContextMenuRequested(const QPoint &pos);

private:
    QVector<QKeySequenceEdit *> keySequenceEdit_vec;
    QList<QKeySequence> keySequence_list;   // Used to track changes

    void addNewKeySequenceEdit();
    void alignKeySequencesLeft();
    void setFocusToLastNonEmptyKeySequenceEdit();

private slots:
    void onEditingFinished();
    void showDefaultContextMenu(QLineEdit *lineEdit, const QPoint &pos);
};

#endif // MULTIKEYEDIT_H
