#ifndef CUSTOMATTRIBUTESDIALOG_H
#define CUSTOMATTRIBUTESDIALOG_H

#include <QDialog>
#include <QAbstractButton>

#include "customattributestable.h"

namespace Ui {
class CustomAttributesDialog;
}

class CustomAttributesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomAttributesDialog(CustomAttributesTable *table);
    ~CustomAttributesDialog();

private:
    Ui::CustomAttributesDialog *ui;
    CustomAttributesTable *const m_table;

    void setInputType(int inputType);
    void onNameChanged(const QString &);
    bool validateName(bool allowEmpty = false);
    void clickedButton(QAbstractButton *button);
    void addNewAttribute();
    QVariant getValue() const;
};


#endif // CUSTOMATTRIBUTESDIALOG_H
