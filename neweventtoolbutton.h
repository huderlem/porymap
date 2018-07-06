#ifndef NEWEVENTTOOLBUTTON_H
#define NEWEVENTTOOLBUTTON_H

#include "event.h"
#include <QToolButton>

class NewEventToolButton : public QToolButton
{
    Q_OBJECT
public:
    explicit NewEventToolButton(QWidget *parent = 0);
    void initButton();
    QString getSelectedEventType();
public slots:
    void newObject();
    void newWarp();
    void newCoordScript();
    void newCoordWeather();
    void newSign();
    void newHiddenItem();
    void newSecretBase();
signals:
    void newEventAdded(QString);
private:
    QString selectedEventType;
    QAction *newObjectAction;
    QAction *newWarpAction;
    QAction *newCoordScriptAction;
    QAction *newCoordWeatherAction;
    QAction *newSignAction;
    QAction *newHiddenItemAction;
    QAction *newSecretBaseAction;
};

#endif // NEWEVENTTOOLBUTTON_H
