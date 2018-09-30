#ifndef TILESETEDITOR_H
#define TILESETEDITOR_H

#include <QDialog>

namespace Ui {
class TilesetEditor;
}

class TilesetEditor : public QDialog
{
    Q_OBJECT

public:
    explicit TilesetEditor(QWidget *parent = nullptr);
    ~TilesetEditor();

private:
    Ui::TilesetEditor *ui;
};

#endif // TILESETEDITOR_H
