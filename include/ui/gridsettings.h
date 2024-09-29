#ifndef GRIDSETTINGS_H
#define GRIDSETTINGS_H

#include <QDialog>
#include <QAbstractButton>

class GridSettings {
public:
    explicit GridSettings() {};
    ~GridSettings() {};

    enum Style {
        Solid,
        LargeDashes,
        SmallDashes,
        Crosshairs,
        Dots,
    };

    uint width = 16;
    uint height = 16;
    int offsetX = 0;
    int offsetY = 0;
    Style style = Style::Solid;
    QColor color = Qt::black;
    QList<qreal> getHorizontalDashPattern() const { return this->getDashPattern(this->width); }
    QList<qreal> getVerticalDashPattern() const { return this->getDashPattern(this->height); }

    static QString getStyleName(Style style);
    static GridSettings::Style getStyleFromName(const QString &name);
private:
    static const QMap<Style, QString> styleToName;

    QList<qreal> getCenteredDashPattern(uint length, qreal dashLength, qreal gapLength) const;
    QList<qreal> getDashPattern(uint length) const;
};

inline bool operator==(const GridSettings &a, const GridSettings &b) {
    return a.width == b.width
        && a.height == b.height
        && a.offsetX == b.offsetX
        && a.offsetY == b.offsetY
        && a.style == b.style
        && a.color == b.color;
}

inline bool operator!=(const GridSettings &a, const GridSettings &b) {
    return !(operator==(a, b));
}



namespace Ui {
class GridSettingsDialog;
}

class GridSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit GridSettingsDialog(QWidget *parent = nullptr);
    explicit GridSettingsDialog(GridSettings *settings, QWidget *parent = nullptr);
    ~GridSettingsDialog();

    void setSettings(const GridSettings &settings);
    GridSettings settings() const { return *m_settings; }

    void setDefaultSettings(const GridSettings &settings);
    GridSettings defaultSettings() const { return m_defaultSettings; }

signals:
    void changedGridSettings();

private:
    Ui::GridSettingsDialog *ui;
    GridSettings *const m_settings;
    const GridSettings m_originalSettings;
    GridSettings m_defaultSettings;
    bool m_dimensionsLinked = true;
    bool m_offsetsLinked = true;
    bool m_ownedSettings = false;

    void init();
    void updateInput();
    void setWidth(int value);
    void setHeight(int value);
    void setOffsetX(int value);
    void setOffsetY(int value);

private slots:
    void dialogButtonClicked(QAbstractButton *button);
    void on_spinBox_Width_valueChanged(int value);
    void on_spinBox_Height_valueChanged(int value);
    void on_spinBox_X_valueChanged(int value);
    void on_spinBox_Y_valueChanged(int value);
    void on_comboBox_Style_currentTextChanged(const QString &text);
    void onColorChanged(QRgb color);
};

#endif // GRIDSETTINGS_H
