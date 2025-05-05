#ifndef NEWLAYOUTFORM_H
#define NEWLAYOUTFORM_H

#include <QWidget>

#include "maplayout.h"

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

    void setSettings(const Layout::Settings &settings);
    Layout::Settings settings() const;

    void setDisabled(bool disabled);

    bool validate();

private:
    Ui::NewLayoutForm *ui;
    Project *m_project;

    bool validateMapDimensions();
    bool validatePrimaryTileset(bool allowEmpty = false);
    bool validateSecondaryTileset(bool allowEmpty = false);
};

#endif // NEWLAYOUTFORM_H
