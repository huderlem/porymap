#pragma once
#ifndef EVENTRAMES_H
#define EVENTRAMES_H

#include <QtWidgets>

#include "noscrollspinbox.h"
#include "noscrollcombobox.h"
#include "mainwindow.h"

#include "events.h"



class Project;

class EventFrame : public QFrame {
    Q_OBJECT

public:
    EventFrame(Event *event, QWidget *parent = nullptr)
        : QFrame(parent), event(event) { }

    virtual void setup();
    void initCustomAttributesTable();
    virtual void connectSignals(MainWindow *);
    virtual void initialize();
    virtual void populate(Project *project);

    void invalidateConnections();
    void invalidateUi();
    void invalidateValues();

    virtual void setActive(bool active);

public:
    QLabel *label_id;

    QVBoxLayout *layout_main;

    QSpinBox *spinner_id;

    NoScrollSpinBox *spinner_x;
    NoScrollSpinBox *spinner_y;
    NoScrollSpinBox *spinner_z;
    QLabel *hideable_label_z;

    QLabel *label_icon;

    QFrame *frame_contents;
    QVBoxLayout *layout_contents;

protected:
    bool populated = false;
    bool initialized = false;
    bool connected = false;

private:
    Event *event;
};



class ObjectFrame : public EventFrame {
    Q_OBJECT

public:
    ObjectFrame(ObjectEvent *object, QWidget *parent = nullptr)
        : EventFrame(object, parent), object(object) {}

    virtual void setup() override;
    virtual void initialize() override;
    virtual void connectSignals(MainWindow *) override;
    virtual void populate(Project *project) override;

public:
    NoScrollComboBox *combo_sprite;
    NoScrollComboBox *combo_movement;
    NoScrollSpinBox *spinner_radius_x;
    NoScrollSpinBox *spinner_radius_y;
    NoScrollComboBox *combo_script;
    QToolButton *button_script;
    NoScrollComboBox *combo_flag;
    NoScrollComboBox *combo_trainer_type;
    NoScrollComboBox *combo_radius_treeid;
    QCheckBox *check_in_connection;

private:
    ObjectEvent *object;

    QCompleter *scriptCompleter = nullptr;
};



class CloneObjectFrame : public EventFrame {
    Q_OBJECT

public:
    CloneObjectFrame(CloneObjectEvent *clone, QWidget *parent = nullptr)
        : EventFrame(clone, parent), clone(clone) {}

    virtual void setup() override;
    virtual void initialize() override;
    virtual void connectSignals(MainWindow *) override;
    virtual void populate(Project *project) override;

public:
    NoScrollComboBox *combo_sprite;
    NoScrollSpinBox *spinner_target_id;
    NoScrollComboBox *combo_target_map;

private:
    CloneObjectEvent *clone;
};



class WarpFrame : public EventFrame {
    Q_OBJECT

public:
    WarpFrame(WarpEvent *warp, QWidget *parent = nullptr)
        : EventFrame(warp, parent), warp(warp) {}

    virtual void setup() override;
    virtual void initialize() override;
    virtual void connectSignals(MainWindow *) override;
    virtual void populate(Project *project) override;

public:
    NoScrollComboBox *combo_dest_map;
    NoScrollComboBox *combo_dest_warp;
    QPushButton *warning;

private:
    WarpEvent *warp;
};



class TriggerFrame : public EventFrame {
    Q_OBJECT

public:
    TriggerFrame(TriggerEvent *trigger, QWidget *parent = nullptr)
        : EventFrame(trigger, parent), trigger(trigger) {}

    virtual void setup() override;
    virtual void initialize() override;
    virtual void connectSignals(MainWindow *) override;
    virtual void populate(Project *project) override;

public:
    NoScrollComboBox *combo_script;
    NoScrollComboBox *combo_var;
    NoScrollComboBox *combo_var_value;

private:
    TriggerEvent *trigger;

    QCompleter *scriptCompleter = nullptr;
};



class WeatherTriggerFrame : public EventFrame {
    Q_OBJECT

public:
    WeatherTriggerFrame(WeatherTriggerEvent *weatherTrigger, QWidget *parent = nullptr)
        : EventFrame(weatherTrigger, parent), weatherTrigger(weatherTrigger) {}

    virtual void setup() override;
    virtual void initialize() override;
    virtual void connectSignals(MainWindow *) override;
    virtual void populate(Project *project) override;

public:
    NoScrollComboBox *combo_weather;

private:
    WeatherTriggerEvent *weatherTrigger;
};



class SignFrame : public EventFrame {
    Q_OBJECT

public:
    SignFrame(SignEvent *sign, QWidget *parent = nullptr)
        : EventFrame(sign, parent), sign(sign) {}

    virtual void setup() override;
    virtual void initialize() override;
    virtual void connectSignals(MainWindow *) override;
    virtual void populate(Project *project) override;

public:
    NoScrollComboBox *combo_facing_dir;
    NoScrollComboBox *combo_script;

private:
    SignEvent *sign;

    QCompleter *scriptCompleter = nullptr;
};



class HiddenItemFrame : public EventFrame {
    Q_OBJECT

public:
    HiddenItemFrame(HiddenItemEvent *hiddenItem, QWidget *parent = nullptr)
        : EventFrame(hiddenItem, parent), hiddenItem(hiddenItem) {}

    virtual void setup() override;
    virtual void initialize() override;
    virtual void connectSignals(MainWindow *) override;
    virtual void populate(Project *project) override;

public:
    QFrame *hideable_quantity;
    QFrame *hideable_itemfinder;
    NoScrollComboBox *combo_item;
    NoScrollComboBox *combo_flag;
    NoScrollSpinBox *spinner_quantity;
    QCheckBox *check_itemfinder;

private:
    HiddenItemEvent *hiddenItem;
};



class SecretBaseFrame : public EventFrame {
    Q_OBJECT

public:
    SecretBaseFrame(SecretBaseEvent *secretBase, QWidget *parent = nullptr)
        : EventFrame(secretBase, parent), secretBase(secretBase) {}

    virtual void setup() override;
    virtual void initialize() override;
    virtual void connectSignals(MainWindow *) override;
    virtual void populate(Project *project) override;

public:
    NoScrollComboBox *combo_base_id;

private:
    SecretBaseEvent *secretBase;
};



class HealLocationFrame : public EventFrame {
    Q_OBJECT

public:
    HealLocationFrame(HealLocationEvent *healLocation, QWidget *parent = nullptr)
        : EventFrame(healLocation, parent), healLocation(healLocation) {}

    virtual void setup() override;
    virtual void initialize() override;
    virtual void connectSignals(MainWindow *) override;
    virtual void populate(Project *project) override;

public:
    QFrame *hideable_respawn_map;
    QFrame *hideable_respawn_npc;
    NoScrollComboBox *combo_respawn_map;
    NoScrollSpinBox *spinner_respawn_npc;

private:
    HealLocationEvent *healLocation;
};

#endif // EVENTRAMES_H
