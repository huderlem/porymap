#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QMainWindow>

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

private:
    Ui::PreferenceEditor *ui;

    void populateFields();
    void saveFields();

private slots:
    void dialogButtonClicked(QAbstractButton *button);
};

#endif // PREFERENCES_H
