#ifndef GRIDSETTINGSDIALOG_H
#define GRIDSETTINGSDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class GridSettingsDialog;
}

struct GridSettings {
	uint width = 16;
	uint height = 16;
	int offsetX = 0;
	int offsetY = 0;
	QString style;
	QColor color;
};


class GridSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit GridSettingsDialog(GridSettings *settings = nullptr, QWidget *parent = nullptr);
    ~GridSettingsDialog();

signals:
    void changedGridSettings();

private:
    Ui::GridSettingsDialog *ui;
    GridSettings *settings;
    GridSettings originalSettings;

    void reset(bool force = false);

private slots:
    void dialogButtonClicked(QAbstractButton *button);
    void on_spinBox_Width_valueChanged(int value);
    void on_spinBox_Height_valueChanged(int value);
    void on_spinBox_X_valueChanged(int value);
    void on_spinBox_Y_valueChanged(int value);
    void on_comboBox_Style_currentTextChanged(QString style);
};

inline bool operator==(const struct GridSettings &a, const struct GridSettings &b) {
    return a.width == b.width
        && a.height == b.height
        && a.offsetX == b.offsetX
        && a.offsetY == b.offsetY
        && a.style == b.style
        && a.color == b.color;
}

inline bool operator!=(const struct GridSettings &a, const struct GridSettings &b) {
    return !(operator==(a, b));
}

#endif // GRIDSETTINGSDIALOG_H
