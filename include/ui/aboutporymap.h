#ifndef ABOUTPORYMAP_H
#define ABOUTPORYMAP_H

#include <QString>
#include <QRegularExpression>
#include <QMainWindow>

namespace Ui {
class AboutPorymap;
}

class AboutPorymap : public QMainWindow
{
public:
    explicit AboutPorymap(QWidget *parent = nullptr);
    ~AboutPorymap();
    QList<int> getVersionNumbers();
private:
    Ui::AboutPorymap *ui;
};

#endif // ABOUTPORYMAP_H
