#ifndef NEWLAYOUTFORM_H
#define NEWLAYOUTFORM_H

#include <QWidget>

class Project;

namespace Ui {
class NewLayoutForm;
}

class NewLayoutForm : public QWidget
{
    Q_OBJECT

public:
    explicit NewLayoutForm(QWidget *parent = nullptr);
    ~NewLayoutForm();

    void initUi(Project *project);

    struct Settings {
        QString id; // TODO: Support in UI (toggleable line edit)
        int width;
        int height;
        int borderWidth;
        int borderHeight;
        QString primaryTilesetLabel;
        QString secondaryTilesetLabel;
    };

    void setSettings(const Settings &settings);
    NewLayoutForm::Settings settings() const;

    void setDisabled(bool disabled);

    bool validate();

private:
    Ui::NewLayoutForm *ui;
    Project *m_project;

    bool validateMapDimensions();
    bool validateTilesets();    
};

#endif // NEWLAYOUTFORM_H
