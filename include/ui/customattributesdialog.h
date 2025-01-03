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
    explicit CustomAttributesDialog(CustomAttributesTable *parent);
    ~CustomAttributesDialog();

    void setInput(const QString &key, QJsonValue value, bool setDefault);

private:
    Ui::CustomAttributesDialog *ui;
    CustomAttributesTable *const table;

    void addNewAttribute();
    bool verifyInput();
    QVariant getValue() const;
    void setNameEditHighlight(bool highlight);
};


#endif // CUSTOMATTRIBUTESDIALOG_H
