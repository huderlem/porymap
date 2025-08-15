#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QMainWindow>
#include "config.h"

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
    void scriptSettingsChanged(ScriptAutocompleteMode mode);
    void reloadProjectRequested();

private:
    Ui::PreferenceEditor *ui;
    QFont applicationFont;
    QFont mapListFont;

    void initFields();
    void saveFields();
    void openFontDialog(QFont *font);

private slots:
    void dialogButtonClicked(QAbstractButton *button);
};

#endif // PREFERENCES_H
