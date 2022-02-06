#ifndef NEWEVENTTOOLBUTTON_H
#define NEWEVENTTOOLBUTTON_H

#include "event.h"
#include <QToolButton>

class NewEventToolButton : public QToolButton
{
    Q_OBJECT
public:
    explicit NewEventToolButton(QWidget *parent = nullptr);
    QString getSelectedEventType();
    QAction *newObjectAction;
    QAction *newCloneObjectAction;
    QAction *newWarpAction;
    QAction *newHealLocationAction;
    QAction *newTriggerAction;
    QAction *newWeatherTriggerAction;
    QAction *newSignAction;
    QAction *newHiddenItemAction;
    QAction *newSecretBaseAction;
public slots:
    void newObject();
    void newCloneObject();
    void newWarp();
    void newHealLocation();
    void newTrigger();
    void newWeatherTrigger();
    void newSign();
    void newHiddenItem();
    void newSecretBase();
signals:
    void newEventAdded(QString);
private:
    QString selectedEventType;
    void init();
};

#endif // NEWEVENTTOOLBUTTON_H
