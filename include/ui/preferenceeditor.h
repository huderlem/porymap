#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QMainWindow>

class NoScrollComboBox;
class QAbstractButton;


namespace Ui {
class PreferenceEditor;
}

class PreferenceEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit PreferenceEditor(QWidget *parent = nullptr);
    ~PreferenceEditor();
    void updateFields();

signals:
    void preferencesSaved();
    void themeChanged(const QString &theme);

private:
    Ui::PreferenceEditor *ui;
    NoScrollComboBox *themeSelector;

    void initFields();
    void saveFields();


private slots:
    void dialogButtonClicked(QAbstractButton *button);
};

#endif // PREFERENCES_H
