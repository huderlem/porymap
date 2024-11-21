#ifndef MAPHEADERFORM_H
#define MAPHEADERFORM_H

/*
    This is the UI class used to edit the fields in a map's header.
    It's intended to be used anywhere the UI needs to present an editor for a map's header,
    e.g. for the current map in the main editor or in the new map dialog.
*/

#include <QWidget>
#include <QPointer>
#include "mapheader.h"

class Project;

namespace Ui {
class MapHeaderForm;
}

class MapHeaderForm : public QWidget
{
    Q_OBJECT

public:
    explicit MapHeaderForm(QWidget *parent = nullptr);
    ~MapHeaderForm();

    void init(const Project * project);
    void clear();

    void setHeader(MapHeader *header);
    MapHeader headerData() const;

    void setLocations(QStringList locations);
    void setLocationDisabled(bool disabled);
    bool isLocationDisabled() const { return m_locationDisabled; }

private:
    Ui::MapHeaderForm *ui;
    QPointer<MapHeader> m_header = nullptr;
    bool m_locationDisabled = false;

    void updateUi();
    void updateSong();
    void updateLocation();
    void updateRequiresFlash();
    void updateWeather();
    void updateType();
    void updateBattleScene();
    void updateShowsLocationName();
    void updateAllowsRunning();
    void updateAllowsBiking();
    void updateAllowsEscaping();
    void updateFloorNumber();

    void onSongUpdated(const QString &song);
    void onLocationChanged(const QString &location);
    void onWeatherChanged(const QString &weather);
    void onTypeChanged(const QString &type);
    void onBattleSceneChanged(const QString &battleScene);
    void onRequiresFlashChanged(int selected);
    void onShowLocationNameChanged(int selected);
    void onAllowRunningChanged(int selected);
    void onAllowBikingChanged(int selected);
    void onAllowEscapingChanged(int selected);
    void onFloorNumberChanged(int offset);
};

#endif // MAPHEADERFORM_H
