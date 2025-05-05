#ifndef ABOUTPORYMAP_H
#define ABOUTPORYMAP_H

#include <QString>
#include <QDialog>

namespace Ui {
class AboutPorymap;
}

class AboutPorymap : public QDialog
{
public:
    explicit AboutPorymap(QWidget *parent = nullptr);
    ~AboutPorymap();

    static QString getVersionString();
private:
    Ui::AboutPorymap *ui;
};

#endif // ABOUTPORYMAP_H
