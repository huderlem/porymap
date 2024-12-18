#ifndef ABOUTPORYMAP_H
#define ABOUTPORYMAP_H

#include <QString>
#include <QRegularExpression>
#include <QDialog>

namespace Ui {
class AboutPorymap;
}

class AboutPorymap : public QDialog
{
public:
    explicit AboutPorymap(QWidget *parent = nullptr);
    ~AboutPorymap();
private:
    Ui::AboutPorymap *ui;
};

#endif // ABOUTPORYMAP_H
